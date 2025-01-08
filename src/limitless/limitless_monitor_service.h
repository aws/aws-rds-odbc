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

enum LIMITLESS_MONITOR_INTERVAL_MS {
    DEFAULT_LIMITLESS_MONITOR_INTERVAL_MS = 1000,
    TEST_LIMITLESS_MONITOR_INTERVAL_MS = 250
};

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

    bool NewService(const std::string& service_id, const SQLTCHAR *connection_string_c_str, int host_port, std::shared_ptr<LimitlessRouterMonitor> limitless_router_monitor);

    void IncrementReferenceCounter(const std::string& service_id);

    void DecrementReferenceCounter(const std::string& service_id);

    std::shared_ptr<HostInfo> GetHostInfo(const std::string& service_id);
private:
    std::map<std::string, std::shared_ptr<LimitlessMonitor>> services;

    std::shared_ptr<std::mutex> services_mutex;

    RoundRobinHostSelector round_robin;
};

#ifdef UNICODE
#define LIMITLESS_MODE_KEY          L"LIMITLESSMODE"
#define LIMITLESS_MODE_VALUE_LAZY   L"lazy"
#else
#define LIMITLESS_MODE_KEY          "LIMITLESSMODE"
#define LIMITLESS_MODE_VALUE_LAZY   "lazy"
#endif

extern "C" {
#endif

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
 * @param service_id_c_str an identifier used to track the reference count of the polling thread
 * @param db_instance a struct containing allocated memory space to return a transaction router
 * @return True if a transaction router was found and updated the LimitlessInstance object
 */
bool GetLimitlessInstance(const SQLTCHAR *connection_string_c_str, int host_port, const char *service_id_c_str, const LimitlessInstance *db_instance);

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
