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

#ifndef SECRETSMANAGERHELPER_H_
#define SECRETSMANAGERHELPER_H_

#include <aws/secretsmanager/SecretsManagerClient.h>

class SecretsManagerHelper {
    public:
        explicit SecretsManagerHelper(std::shared_ptr<Aws::SecretsManager::SecretsManagerClient> sm_client)
            : sm_client(std::move(sm_client)) {}
        ~SecretsManagerHelper() = default;

        // Tries to parse region from secret id or from user input, else default to us-east-1
        static void ParseRegionFromSecretId(const Aws::String& secret_id, Aws::String& region);

        // fetches credentials from the configured secrets manager client and stored the retrieved values into username and password
        bool FetchCredentials(const Aws::String& secret_id);

        // get the fetched username
        Aws::String GetUsername() { return this->username; }

        // get the fetched password
        Aws::String GetPassword() { return this->password; }
    private:
        std::shared_ptr<Aws::SecretsManager::SecretsManagerClient> sm_client;
        Aws::String username;
        Aws::String password;
};

#endif // SECRETSMANAGERHELPER_H_
