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
#include "limitless/limitless_monitor_service.h"

#define MAX_SERVER_HOST_SIZE 1024

int main(int argc, char* argv[]) {
    SQLHENV henv;
    SQLHDBC hdbc;

    SQLAllocHandle(SQL_HANDLE_ENV, NULL, &henv);
    SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);
    SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);

    const char* conn_str = "DSN=sample_DSN;SERVER=sample_Server;PORT=3306;UID=sample_User;PWD=sample_Pwd;DATABASE=sample_Db;";
    int server_port = 5432;
    // Set a custom service ID such that all routers can map to the same cluster
    char* service_id = "custom_id";

    // Check if a connection is a Limitless Cluster or not
    bool is_conn_limitless = CheckLimitlessCluster(hdbc);
    if (is_conn_limitless) {
        // Allocate space to get a server host
        LimitlessInstance limitless_info;
        limitless_info.server = (char*) malloc(MAX_SERVER_HOST_SIZE);
        limitless_info.server_size = MAX_SERVER_HOST_SIZE;

        // Call to get a Limitless router and based off of the service ID to periodically update the list of live routers
        // Each call will increment the reference count, and will be joined when reference counts reaches 0
        bool has_limitless_instance = GetLimitlessInstance((SQLTCHAR *) conn_str, server_port, service_id, MAX_SERVER_HOST_SIZE, &limitless_info);

        // ... do things with new server host...

        // Cleanup
        free(limitless_info.server);
        // Decrements reference count for thread running for the given service ID
        // Once reference count reaches 0, the thread will stop
        StopLimitlessMonitorService(service_id);
    } else {
        // Not a Limitless Cluster!
    }
}
