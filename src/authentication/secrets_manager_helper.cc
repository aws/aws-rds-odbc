//  Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Library General Public
//  License as published by the Free Software Foundation; either
//  version 2 of the License, or (at your option) any later version.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Library General Public License for more details.
//
//  You should have received a copy of the GNU Library General Public
//  License along with this library; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

#include "secrets_manager_helper.h"

#include <cstdlib>
#include <cstring>
#include <regex>

#include <aws/core/Aws.h>
#include <aws/core/utils/json/JsonSerializer.h>
#include <aws/secretsmanager/SecretsManagerServiceClientModel.h>
#include <aws/secretsmanager/model/GetSecretValueRequest.h>

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
