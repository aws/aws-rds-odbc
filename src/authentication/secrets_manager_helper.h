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
