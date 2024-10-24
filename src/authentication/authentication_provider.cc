// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License, version 2.0
// (GPLv2), as published by the Free Software Foundation, with the
// following additional permissions:
//
// This program is distributed with certain software that is licensed
// under separate terms, as designated in a particular file or component
// or in the license documentation. Without limiting your rights under
// the GPLv2, the authors of this program hereby grant you an additional
// permission to link the program and your derivative works with the
// separately licensed software that they have included with the program.
//
// Without limiting the foregoing grant of rights under the GPLv2 and
// additional permission as to separately licensed software, this
// program is also subject to the Universal FOSS Exception, version 1.0,
// a copy of which can be found along with its FAQ at
// http://oss.oracle.com/licenses/universal-foss-exception.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License, version 2.0, for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see 
// http://www.gnu.org/licenses/gpl-2.0.html.

#include "authentication_provider.h"

#include "adfs/adfs.h"

#ifndef XCODE_BUILD
#include "../util/logger_wrapper.h"
#endif

#include <aws/core/auth/AWSCredentials.h>
#include <aws/core/auth/AWSCredentialsProviderChain.h>
#include <aws/core/http/HttpClient.h>
#include <aws/core/Aws.h>
#include <aws/sts/model/AssumeRoleWithSAMLRequest.h>
#include <aws/sts/STSClient.h>
#include <aws/rds/RDSClient.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <mutex>
#include <string>
#include <unordered_map>

Aws::SDKOptions sdkOptions;
std::atomic<int> sdkRefCount{0};
std::mutex sdkMutex;

typedef struct tokenInfo {
    std::string token;
    long        expiration;  // the expiration time in second
} TokenInfo;

// Cached TokenInfo
std::unordered_map<std::string, TokenInfo> cachedTokens;

bool ValidateCharArr(std::string_view str) {
    return !str.empty();
}

bool ValidateAdfsConf(FederatedAuthConfig config) {
    return ValidateCharArr(config.idp_endpoint) &&
        ValidateCharArr(config.idp_port) &&
        ValidateCharArr(config.relaying_party_id) &&
        ValidateCharArr(config.iam_role_arn) &&
        ValidateCharArr(config.iam_idp_arn) &&
        ValidateCharArr(config.idp_username) &&
        ValidateCharArr(config.idp_password);
}

Aws::RDS::RDSClient* CreateRDSClient(FederatedAuthType type, FederatedAuthConfig config) {
    Aws::Auth::AWSCredentials credentials;
    switch (type) {
        case ADFS: {
            if (!ValidateAdfsConf(config)) {
                #ifndef XCODE_BUILD
                LOG(ERROR) << "Configuration for " << FEDERATEDAUTHTYPE_STRING[type] << " is invalid/incomplete.";
                #endif
                return nullptr;
            };
            Aws::Client::ClientConfiguration httpClientCfg;
            httpClientCfg.requestTimeoutMs = config.http_client_connect_timeout ? atol(config.http_client_connect_timeout) : 3000;
            httpClientCfg.connectTimeoutMs = config.http_client_connect_timeout ? atol(config.http_client_connect_timeout) : 5000;
            httpClientCfg.verifySSL = true;
            std::shared_ptr<Aws::Http::HttpClient> httpClient = Aws::Http::CreateHttpClient(httpClientCfg);
            std::shared_ptr<Aws::STS::STSClient> stsClient = std::make_shared<Aws::STS::STSClient>();
            AdfsCredentialsProvider adfs(config, httpClient, stsClient);
            if (!adfs.GetAWSCredentials(credentials)) {
                #ifndef XCODE_BUILD
                LOG(ERROR) << FEDERATEDAUTHTYPE_STRING[type] << " provider failed to get valid credentials.";
                #endif
                return nullptr;
            }
            break;
        }
        case IAM: {
            Aws::Auth::DefaultAWSCredentialsProviderChain credProvider;
            credentials = credProvider.GetAWSCredentials();
            break;
        }
        case OKTA:
            // Fallthru, not implemented
        default:
            #ifndef XCODE_BUILD
            LOG(ERROR) << FEDERATEDAUTHTYPE_STRING[type] << " is not a valid authentication type.";
            #endif
            return nullptr;
    }
    Aws::RDS::RDSClientConfiguration rdsClientCfg;
    return new Aws::RDS::RDSClient(credentials, rdsClientCfg);
}

void FreeAwsResource(Aws::RDS::RDSClient* client) {
    if (client) {
        delete static_cast<Aws::RDS::RDSClient*>(client);
    }

    // Shut down AWS API
    if (0 == --sdkRefCount) {
        std::lock_guard<std::mutex> lock(sdkMutex);
        Aws::ShutdownAPI(sdkOptions);
    }
}

void GenKeyAndTime(const char* dbHostName, const char* dbRegion, const char* port, const char* dbUserName, std::string& key, long& currentTimeInSeconds) {
    key = std::string(dbHostName);
    key += "-";
    key += std::string(dbRegion);
    key += "-";
    key += std::string(port);
    key += "-";
    key += std::string(dbUserName);

    // Get the current time point
    auto currentTimePoint = std::chrono::system_clock::now();

    // Convert the time point to seconds
    currentTimeInSeconds = std::chrono::duration_cast<std::chrono::seconds>(currentTimePoint.time_since_epoch()).count();
}

bool UpdateTokenValue(char* token, const int maxSize, const char* newValue) {
    int newTokenSize = strlen(newValue);
    if (maxSize - 1 < newTokenSize) {
        #ifndef XCODE_BUILD
        LOG(WARNING) << "New token does not fit into allocated token";
        #endif
        return false;
    }

    memcpy(token, newValue, newTokenSize);
    token[newTokenSize] = 0;
    return true;
}

#ifdef __cplusplus
extern "C" {
#endif

FederatedAuthType GetFedAuthTypeEnum(const char *str) {
    std::string upperStr(str);
    std::transform(upperStr.begin(), upperStr.end(), upperStr.begin(), ::toupper);
    for (int i = 0; i < INVALID; i++) {
        if (!upperStr.compare(FEDERATEDAUTHTYPE_STRING[i])) {
            return (FederatedAuthType)i;
        }
    }
    return INVALID;
}

bool GetCachedToken(char* token, const int maxSize, const char* dbHostName, const char* dbRegion, const char* port, const char* dbUserName) {
    std::string key("");
    long currentTimeInSeconds = 0;
    GenKeyAndTime(dbHostName, dbRegion, port, dbUserName, key, currentTimeInSeconds);

    auto itr = cachedTokens.find(key);
    if (itr == cachedTokens.end() || currentTimeInSeconds > itr->second.expiration) {
        #ifndef XCODE_BUILD
        LOG(WARNING) << "No cached token";
        #endif
        return false;
    } else {
        int tokenSize = itr->second.token.size();
        #ifndef XCODE_BUILD
        LOG(INFO) << "Token size is " << tokenSize;
        #endif
        return UpdateTokenValue(token, maxSize, itr->second.token.c_str());
    }
}

void UpdateCachedToken(const char* dbHostName, const char* dbRegion, const char* port, const char* dbUserName, const char* token, const char* expirationTime) {
    std::string key("");
    long currentTimeInSeconds = 0;
    GenKeyAndTime(dbHostName, dbRegion, port, dbUserName, key, currentTimeInSeconds);

    TokenInfo ti;
    ti.token = std::string(token);
    ti.expiration = currentTimeInSeconds + atol(expirationTime);
    cachedTokens[key] = ti;
}

bool GenerateConnectAuthToken(char* token, const int maxSize, const char* dbHostName, const char* dbRegion, unsigned port, const char* dbUserName, FederatedAuthType type, FederatedAuthConfig config) {
    // TODO - Need to move logger initializer to a central location
    #ifndef XCODE_BUILD
    LOGGER_WRAPPER::initialize();
    #endif
    if (1 == ++sdkRefCount) {
        std::lock_guard<std::mutex> lock(sdkMutex);
        Aws::InitAPI(sdkOptions);
    }

    #ifndef XCODE_BUILD
    LOG(INFO) << "Generating token for " << FEDERATEDAUTHTYPE_STRING[type];
    #endif

    Aws::RDS::RDSClient* client = CreateRDSClient(type, config);
    if (!client) {
        FreeAwsResource(nullptr);
        return false;
    }

    Aws::String newToken = client->GenerateConnectAuthToken(dbHostName, dbRegion, port, dbUserName);
    FreeAwsResource(client);

    int tokenSize = newToken.size();
    #ifndef XCODE_BUILD
    LOG(INFO) << "RDS Client generated token length is " << tokenSize;
    #endif
    return UpdateTokenValue(token, maxSize, newToken.c_str());
}

#ifdef __cplusplus
}
#endif
