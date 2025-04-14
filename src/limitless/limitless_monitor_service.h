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

#ifndef LIMITLESSMONITORSERVICE_H_
#define LIMITLESSMONITORSERVICE_H_

#ifdef __cplusplus
#include <cstdio>
#include <map>
#include <mutex>
#include <string>

#ifdef WIN32
#include <windows.h>
#endif

#include <sql.h>
#include <sqlext.h>

#include "limitless_router_monitor.h"
#include "round_robin_host_selector.h"
#include "../util/text_helper.h"

typedef struct LimitlessMonitor {
    ~LimitlessMonitor() {
        this->limitless_router_monitor = nullptr;
        this->limitless_routers_mutex = nullptr;
        this->limitless_routers = nullptr;
    }

    unsigned int reference_counter;
    std::shared_ptr<std::vector<HostInfo>> limitless_routers;
    std::shared_ptr<std::mutex> limitless_routers_mutex;
    std::shared_ptr<LimitlessRouterMonitor> limitless_router_monitor;
} LimitlessMonitor;

class LimitlessMonitorService {
public:
    LimitlessMonitorService();

    ~LimitlessMonitorService();

    bool CheckService(const std::string& service_id);

    bool NewService(std::string& service_id, const SQLTCHAR *connection_string_c_str, int host_port, std::shared_ptr<LimitlessRouterMonitor> limitless_router_monitor);

    void IncrementReferenceCounter(const std::string& service_id);

    void DecrementReferenceCounter(const std::string& service_id);

    std::shared_ptr<HostInfo> GetHostInfo(const std::string& service_id);
private:
    std::map<std::string, std::shared_ptr<LimitlessMonitor>> services;

    std::shared_ptr<std::mutex> services_mutex;

    RoundRobinHostSelector round_robin;
};

#define LIMITLESS_MODE_VALUE_LAZY TEXT("lazy")

extern "C" {
#endif

#define DEFAULT_LIMITLESS_MONITOR_INTERVAL_MS 7500

typedef struct {
    char *server;
    unsigned int server_size;
} LimitlessInstance;

/**
 * Checks the server of a given connection string if it is a Limitless Cluster
 * by connecting using the ODBC API to connect and check using a query
 *
 * @param connection_string_c_str the connection string to specify the driver and server
 * @return True if the given connection string connects to a Limitless Cluster
 */
bool CheckLimitlessCluster(SQLHDBC hdbc);

/**
 * Increments a reference count for the given service ID, spinning a transaction router polling thread
 * if service ID is unique. Once thread is ready, the given LimitlessInstance, db_instance, will get
 * an updated server host
 *
 * @param connection_string_c_str the connection string to specify the driver and server
 * @param host_port the port to the database, used to create the host list
 * @param service_id_c_str an identifier used to track the reference count of the polling thread - overwritten if empty
 * @param service_id_size size of service_id_c_str to overwrite if service_id_c_str is empty
 * @param db_instance a struct containing allocated memory space to return a transaction router
 * @return True if a transaction router was found and updated the LimitlessInstance object
 */
bool GetLimitlessInstance(const SQLTCHAR *connection_string_c_str, int host_port, char *service_id_c_str, size_t service_id_size, const LimitlessInstance *db_instance);

/**
 * Decrements the reference count of a given service ID.
 * Once it reaches 0, the polling thread will be joined and cleaned up
 *
 * @param service_id_c_str service_id_c_str an identifier used to track the reference count of the polling thread
 */
void StopLimitlessMonitorService(const char *service_id_c_str);

#ifdef __cplusplus
}
#endif

#endif // LIMITLESSMONITORSERVICE_H_
