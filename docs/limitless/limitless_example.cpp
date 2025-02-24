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
