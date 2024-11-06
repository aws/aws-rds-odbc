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

#include <cstdlib>
#include <cstring>
#include <regex>

#include <aws/core/Aws.h>
#include <aws/core/utils/json/JsonSerializer.h>
#include <aws/secretsmanager/model/GetSecretValueRequest.h>
#include <aws/secretsmanager/SecretsManagerServiceClientModel.h>

#include "authentication_provider.h"
#include "secrets_manager_helper.h"

#ifndef XCODE_BUILD
#include "../util/logger_wrapper.h"
#endif

namespace {
    const Aws::String USERNAME_KEY{ "username" };
    const Aws::String PASSWORD_KEY{ "password" };
    const std::string SECRETS_ARN_PATTERN{ "arn:aws:secretsmanager:([-a-zA-Z0-9]+):.*" };
}

bool SECRETS_MANAGER_HELPER::TryParseRegionFromSecretId(const Aws::String& secret_id, Aws::String& region) {
    std::regex rgx(SECRETS_ARN_PATTERN);
    std::smatch matches;

    if (std::regex_search(secret_id, matches, rgx) && matches.size() > 1) {
        region = matches[1].str();
        return true;
    }

    return false;
}

bool SECRETS_MANAGER_HELPER::FetchCredentials(const Aws::String& secret_id) {
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

        return false;
    } else {
        LOG(ERROR) << "Error getting secret value: " << outcome.GetError().GetMessage();
        return false;
    }
}
