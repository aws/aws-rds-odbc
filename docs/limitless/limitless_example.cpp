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

// Include required header
#include "limitless/limitless_monitor_service.h"

#define MAX_SERVER_HOST_SIZE 1024

int main(int argc, char* argv[]) {
    const char* conn_str = "DSN=sample_DSN;SERVER=sample_Server;PORT=3306;UID=sample_User;PWD=sample_Pwd;DATABASE=sample_Db;";
    int server_port = 5432;
    // Set a custom service ID such that all routers can map to the same cluster
    const char* service_id = "custom_id";

    // Check if a connection is a Limitless Cluster or not
    bool is_conn_limitless = CheckLimitlessCluster();
    if (is_conn_limitless) {
        // Allocate space to get a server host
        LimitlessInstance limitless_info;
        limitless_info.server = (char*) malloc(MAX_SERVER_HOST_SIZE);
        limitless_info.server_size = MAX_SERVER_HOST_SIZE;

        // Call to get a Limitless router and based off of the service ID to periodically update the list of live routers
        // Each call will increment the reference count, and will be joined when reference counts reaches 0
        bool has_limitless_instance = GetLimitlessInstance(conn_str, server_port, service_id, &limitless_info);

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
