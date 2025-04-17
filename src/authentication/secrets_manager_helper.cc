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

#include "secrets_manager_helper.h"

#include <cstdlib>
#include <cstring>
#include <regex>

#include <aws/core/Aws.h>
#include <aws/core/utils/json/JsonSerializer.h>
#include <aws/secretsmanager/SecretsManagerServiceClientModel.h>
#include <aws/secretsmanager/model/GetSecretValueRequest.h>
#include <glog/logging.h>

#include "../util/logger_wrapper.h"
#include "authentication_provider.h"

namespace {
    const Aws::String USERNAME_KEY{ "username" };
    const Aws::String PASSWORD_KEY{ "password" };
    const std::string SECRETS_ARN_PATTERN{ "arn:aws:secretsmanager:([-a-zA-Z0-9]+):.*" };
}

void SecretsManagerHelper::ParseRegionFromSecretId(const Aws::String& secret_id, Aws::String& region) {
    std::regex rgx(SECRETS_ARN_PATTERN);
    std::smatch matches;

    if (std::regex_search(secret_id, matches, rgx) && matches.size() > 1) {
        // Attempt to parse out region from Secret Id
        // Region from full ARN will guarantee the correct region
        region = matches[1].str();
    } else if (region.empty()) {
        // No region match in secret arn, fallback to user input
        // If user does not supply any value, default to us-east-1
        LOG(WARNING) << "Unable to parse region from SecretId & no passed in region. Defaulting to us-east-1";
        region = Aws::Region::US_EAST_1;
    }
}

bool SecretsManagerHelper::FetchCredentials(const Aws::String& secret_id) {
    Aws::SecretsManager::Model::GetSecretValueRequest request;
    request.SetSecretId(secret_id);

    const Aws::SecretsManager::Model::GetSecretValueOutcome outcome = this->sm_client->GetSecretValue(request);

    if (outcome.IsSuccess()) {
        const Aws::String secret_str = outcome.GetResult().GetSecretString();
        const Aws::Utils::Json::JsonValue secret_json = Aws::Utils::Json::JsonValue(secret_str);
        const Aws::Utils::Json::JsonView view = secret_json.View();

        if (view.ValueExists(USERNAME_KEY) && view.ValueExists(PASSWORD_KEY)) {
            username = view.GetString(USERNAME_KEY);
            password = view.GetString(PASSWORD_KEY);

            return true;
        }
        LOG(ERROR) << "Secret missing expected key value pairs";
        return false;
    }

    LOG(ERROR) << "Error getting secret value: " << outcome.GetError().GetMessage();
    return false;
}
