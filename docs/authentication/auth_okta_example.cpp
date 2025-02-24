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

// Include required header
#include <authentication/authentication_provider.h>

#include <cstring>
#include <stdio.h>
#include <stdlib.h>

#define MAX_TOKEN_SIZE 1024

int main(int argc, char* argv[]) {
    // Specify authentication type [IAM, ADFS, OKTA]
    FederatedAuthType authType = FederatedAuthType::OKTA;

    // Specify authentication configurations if any
    FederatedAuthConfig authConfig;
    std::strcpy(authConfig.idp_endpoint, "example.com");
    std::strcpy(authConfig.idp_port, "443");
    std::strcpy(authConfig.app_id, "my_okta_app_id");
    std::strcpy(authConfig.iam_role_arn, "arn:aws:iam::123412341234:role/okta_example_role");
    std::strcpy(authConfig.iam_idp_arn, "arn:aws:iam::123412341234:saml-provider/okta_example_provider");
    std::strcpy(authConfig.idp_username, "user@email.com");
    std::strcpy(authConfig.idp_password, "my_password");

    // Allocate memory to store the token
    char new_token[MAX_TOKEN_SIZE];

    // Specify DB properties
    char* db_server = "sample-db.cluster-abc.us-west-2.rds.amazonaws.com";
    char* region = "us-west-2";
    char* db_port = "5432";
    char* iam_db_username = "jane_doe";

    // This will generate a new token replace the value of your new token
    GenerateConnectAuthToken(new_token, MAX_TOKEN_SIZE,
        db_server, region, atoi(db_port), iam_db_username,
        authType, authConfig);

    // Can also cache and store the value...
    char* token_expiration = "900";
    UpdateCachedToken(db_server, region, db_port, iam_db_username, new_token, token_expiration);

    // ... and retrieve the value from the cache
    GetCachedToken(new_token, MAX_TOKEN_SIZE, db_server, region, db_port, iam_db_username);

    // ... do things with credentials...
}
