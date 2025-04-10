// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "authentication_provider.h"

#include <adfs/adfs.h>
#include <okta/okta.h>

#include <aws/core/Aws.h>
#include <aws/core/auth/AWSCredentials.h>
#include <aws/core/auth/AWSCredentialsProviderChain.h>
#include <aws/core/http/HttpClient.h>
#include <aws/rds/RDSClient.h>
#include <aws/sts/STSClient.h>
#include <aws/sts/model/AssumeRoleWithSAMLRequest.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <mutex>
#include <string>
#include <unordered_map>

#include "../util/logger_wrapper.h"
#include "../util/string_to_number_converter.h"
#include "secrets_manager_helper.h"

static Aws::SDKOptions sdk_opts;
static std::atomic<int> sdk_ref_count{0};
static std::mutex sdk_mutex;

static std::mutex global_auth_mutex;

static constexpr uint64_t DEFAULT_SOCKET_TIMEOUT = 3000;
static constexpr uint64_t DEFAULT_CONNECT_TIMEOUT = 5000;

struct TokenInfo {
    std::string token;
    uint64_t    expiration;  // the expiration time in second
}; // TokenInfo;

// Cached TokenInfo
static std::unordered_map<std::string, TokenInfo> cached_tokens;

static bool ValidateCharArr(std::string_view str) {
    return !str.empty();
}

static bool ValidateAdfsConf(FederatedAuthConfig config) {
    return ValidateCharArr(config.idp_endpoint) &&
        ValidateCharArr(config.idp_port) &&
        ValidateCharArr(config.relaying_party_id) &&
        ValidateCharArr(config.iam_role_arn) &&
        ValidateCharArr(config.iam_idp_arn) &&
        ValidateCharArr(config.idp_username) &&
        ValidateCharArr(config.idp_password);
}

static bool ValidateOktaConf(FederatedAuthConfig config) {
    return ValidateCharArr(config.idp_endpoint) &&
        ValidateCharArr(config.idp_port) &&
        ValidateCharArr(config.app_id) &&
        ValidateCharArr(config.iam_role_arn) &&
        ValidateCharArr(config.iam_idp_arn) &&
        ValidateCharArr(config.idp_username) &&
        ValidateCharArr(config.idp_password);
}

static uint64_t ParseNumber(const char* str_in, const uint64_t default_val) {
    try {
        return StringToNumberConverter::toLong(str_in);
    } catch (std::exception& e) {
        return default_val;
    }
}

static Aws::RDS::RDSClient* CreateRDSClient(FederatedAuthType type, FederatedAuthConfig config) {
    Aws::Auth::AWSCredentials credentials;
    switch (type) {
        case ADFS: {
            if (!ValidateAdfsConf(config)) {
                LOG(ERROR) << "Configuration for " << federated_auth_type_str[type] << " is invalid/incomplete.";
                return nullptr;
            };
            Aws::Client::ClientConfiguration http_client_cfg;
            http_client_cfg.requestTimeoutMs = ParseNumber(config.http_client_socket_timeout, DEFAULT_SOCKET_TIMEOUT);
            http_client_cfg.connectTimeoutMs = ParseNumber(config.http_client_connect_timeout, DEFAULT_CONNECT_TIMEOUT);
            http_client_cfg.verifySSL = true;
            std::shared_ptr<Aws::Http::HttpClient> http_client = Aws::Http::CreateHttpClient(http_client_cfg);
            std::shared_ptr<Aws::STS::STSClient> sts_client = std::make_shared<Aws::STS::STSClient>();
            AdfsCredentialsProvider adfs(config, http_client, sts_client);
            if (!adfs.GetAWSCredentials(credentials)) {
                LOG(ERROR) << federated_auth_type_str[type] << " provider failed to get valid credentials.";
                return nullptr;
            }
            break;
        }
        case IAM: {
            Aws::Auth::DefaultAWSCredentialsProviderChain cred_provider;
            credentials = cred_provider.GetAWSCredentials();
            break;
        }
        case OKTA: {
            if (!ValidateOktaConf(config)) {
                LOG(ERROR) << "Configuration for " << federated_auth_type_str[type] << " is invalid/incomplete.";
                return nullptr;
            };
            Aws::Client::ClientConfiguration http_client_cfg;
            http_client_cfg.requestTimeoutMs = ParseNumber(config.http_client_socket_timeout, DEFAULT_SOCKET_TIMEOUT);
            http_client_cfg.connectTimeoutMs = ParseNumber(config.http_client_connect_timeout, DEFAULT_CONNECT_TIMEOUT);
            http_client_cfg.verifySSL = true;
            http_client_cfg.followRedirects = Aws::Client::FollowRedirectsPolicy::ALWAYS;
            std::shared_ptr<Aws::Http::HttpClient> http_client = Aws::Http::CreateHttpClient(http_client_cfg);
            std::shared_ptr<Aws::STS::STSClient> sts_client = std::make_shared<Aws::STS::STSClient>();
            OktaCredentialsProvider okta(config, http_client, sts_client);
            if (!okta.GetAWSCredentials(credentials)) {
                LOG(ERROR) << federated_auth_type_str[type] << " provider failed to get valid credentials.";
                return nullptr;
            }
            break;
        }
        default:
            LOG(ERROR) << federated_auth_type_str[type] << " is not a valid authentication type.";
            return nullptr;
    }
    Aws::RDS::RDSClientConfiguration rds_client_cfg;
    return new Aws::RDS::RDSClient(credentials, rds_client_cfg);
}

static void ShutdownAwsAPI() {
    if (0 == --sdk_ref_count) {
        std::lock_guard<std::mutex> lock(sdk_mutex);
        Aws::ShutdownAPI(sdk_opts);
    }
}

static void FreeAwsResource(Aws::RDS::RDSClient* client) {
    delete client;

    // Shut down AWS API
    ShutdownAwsAPI();
}

static void GenKeyAndTime(const char* db_hostname, const char* db_region, const char* port, const char* db_user, std::string& key, uint64_t& curr_time_in_sec) {
    key = std::string(db_hostname);
    key += "-";
    key += std::string(db_region);
    key += "-";
    key += std::string(port);
    key += "-";
    key += std::string(db_user);

    // Get the current time point
    auto curr_time = std::chrono::system_clock::now();

    // Convert the time point to seconds
    curr_time_in_sec = std::chrono::duration_cast<std::chrono::seconds>(curr_time.time_since_epoch()).count();
}

static bool UpdateTokenValue(char* token, const unsigned max_size, const char* new_value) {
    int new_token_size = strlen(new_value);
    if (max_size - 1 < new_token_size) {
        LOG(WARNING) << "New token does not fit into allocated token";
        return false;
    }

    memcpy(token, new_value, new_token_size);
    token[new_token_size] = 0;
    return true;
}

FederatedAuthType GetFedAuthTypeEnum(const char *str) {
    std::string upper_str(str);
    std::transform(upper_str.begin(), upper_str.end(), upper_str.begin(), ::toupper);
    for (int i = 0; i < INVALID; i++) {
        if (upper_str == federated_auth_type_str[i]) {
            return static_cast<FederatedAuthType>(i);
        }
    }
    return INVALID;
}

bool GetCachedToken(char* token, unsigned int max_size, const char* db_hostname, const char* db_region, const char* port, const char* db_user) {
    std::string key;
    uint64_t curr_time_in_sec = 0;
    GenKeyAndTime(db_hostname, db_region, port, db_user, key, curr_time_in_sec);

    auto itr = cached_tokens.find(key);
    if (itr == cached_tokens.end() || curr_time_in_sec > itr->second.expiration) {
        LOG(WARNING) << "No cached token";
        return false;
    }

    int token_size = itr->second.token.size();
    LOG(INFO) << "Token size is " << token_size;
    return UpdateTokenValue(token, max_size, itr->second.token.c_str());
}

void UpdateCachedToken(const char* db_hostname, const char* db_region, const char* port, const char* db_user, const char* token, const char* expiration_time) {
    std::string key;
    uint64_t curr_time_in_sec = 0;
    GenKeyAndTime(db_hostname, db_region, port, db_user, key, curr_time_in_sec);

    TokenInfo ti;
    ti.token = std::string(token);
    ti.expiration = curr_time_in_sec + StringToNumberConverter::toLong(expiration_time);
    cached_tokens[key] = ti;
}

bool GenerateConnectAuthToken(char* token, unsigned int max_size, const char* db_hostname, const char* db_region, unsigned port, const char* db_user, FederatedAuthType type, FederatedAuthConfig config) {
    std::lock_guard<std::mutex> global_lock(global_auth_mutex);
    if (1 == ++sdk_ref_count) {
        std::lock_guard<std::mutex> lock(sdk_mutex);
        Aws::InitAPI(sdk_opts);
    }

    LOG(INFO) << "Generating token for " << federated_auth_type_str[type];

    Aws::RDS::RDSClient* client = CreateRDSClient(type, config);
    if (!client) {
        FreeAwsResource(nullptr);
        return false;
    }

    Aws::String new_token = client->GenerateConnectAuthToken(db_hostname, db_region, port, db_user);
    FreeAwsResource(client);

    int token_size = new_token.size();
    LOG(INFO) << "RDS Client generated token length is " << token_size;
    return UpdateTokenValue(token, max_size, new_token.c_str());
}

bool GetCredentialsFromSecretsManager(const char* secret_id, const char* region, Credentials* credentials) {
    if (1 == ++sdk_ref_count) {
        std::lock_guard<std::mutex> lock(sdk_mutex);
        Aws::InitAPI(sdk_opts);
    }

    std::string region_str = region;
    SecretsManagerHelper::ParseRegionFromSecretId(secret_id, region_str);

    // configure the secrets manager client according to the region determined
    Aws::SecretsManager::SecretsManagerClientConfiguration sm_client_cfg;
    sm_client_cfg.region = region_str;
    std::shared_ptr<Aws::SecretsManager::SecretsManagerClient> sm_client = std::make_shared<Aws::SecretsManager::SecretsManagerClient>(sm_client_cfg);

    SecretsManagerHelper sm_helper(sm_client);
    bool is_success = sm_helper.FetchCredentials(secret_id);
    ShutdownAwsAPI(); // done using the AWS API

    // don't copy any memory if there was a failure fetching the credentials
    if (!is_success) {
        return false;
    }

    return UpdateTokenValue(credentials->username, credentials->username_size, sm_helper.GetUsername().c_str())
        && UpdateTokenValue(credentials->password, credentials->password_size, sm_helper.GetPassword().c_str());
}
