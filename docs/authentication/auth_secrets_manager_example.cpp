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
    // Allocate memory to store the username and password
    Credentials credentials;
    credentials.username = (char*) malloc(MAX_TOKEN_SIZE);
    credentials.password = (char*) malloc(MAX_TOKEN_SIZE);
    credentials.username_size = MAX_TOKEN_SIZE;
    credentials.password_size = MAX_TOKEN_SIZE;

    // Specify Secrets Manager Key
    // This can either full ARN or Secret ID
    char* secret_id = "arn:aws:secretsmanager:us-west-2:123412341234:secret:secret-id";
    // If given a full ARN, the region will automatically be parsed
    // Else, will need to provide the region the secret is stored in
    char* secret_region = "us-west-2";

    GetCredentialsFromSecretsManager(secret_id, secret_region, &credentials);

    // ... do things with credentials...

    // Cleanup
    free(credentials.username);
    free(credentials.password);
}
