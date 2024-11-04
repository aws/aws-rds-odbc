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

#ifndef __SECRETSMANAGERHELPER_H__
#define __SECRETSMANAGERHELPER_H__

#include <aws/secretsmanager/SecretsManagerClient.h>

class SECRETS_MANAGER_HELPER {
    public:
        SECRETS_MANAGER_HELPER(std::shared_ptr<Aws::SecretsManager::SecretsManagerClient> sm_client)
            : sm_client(sm_client) {}
        ~SECRETS_MANAGER_HELPER() {}

        // attempts to match secret_ID to a secrets ARN regex and parses the region portion; returns true if parsed, false if not
        static bool TryParseRegionFromSecretId(const Aws::String& secret_id, Aws::String& region);

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

#endif
