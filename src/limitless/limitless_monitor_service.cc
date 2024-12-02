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

#include <cstring>
#include <map>
#include <mutex>
#include <string>

#include "limitless_monitor_service.h"
#include "limitless_router_monitor.h"
#include "odbc_helper.h"
#include "round_robin_host_selector.h"

enum LIMITLESS_MONITOR_INTERVAL {
    DEFAULT_LIMITLESS_MONITOR_INTERVAL = 1000
};

typedef struct {
    unsigned int depth_counter;
    std::shared_ptr<std::vector<HostInfo>> limitless_routers;
    std::shared_ptr<std::mutex> limitless_routers_mutex;
    std::shared_ptr<LimitlessRouterMonitor> limitless_router_monitor;
} LimitlessMonitorService;

static std::map<std::string, std::shared_ptr<LimitlessMonitorService>> services;
static std::shared_ptr<std::mutex> services_mutex = std::make_shared<std::mutex>();

static RoundRobinHostSelector round_robin;

bool CheckLimitlessCluster(const char *connection_string_c_str) {
    SQLHENV henv = SQL_NULL_HANDLE;
    SQLHDBC conn = SQL_NULL_HANDLE;
    auto *connection_string = const_cast<SQLCHAR *>(reinterpret_cast<const SQLCHAR *>(connection_string_c_str));
    SQLSMALLINT connection_string_len = strlen(connection_string_c_str);
    SQLSMALLINT out_connection_string_len; // unused

    SQLRETURN rc = SQLAllocHandle(SQL_HANDLE_ENV, nullptr, &henv);
    if (!OdbcHelper::CheckResult(rc, "SQLAllocHandle failed", henv, SQL_HANDLE_ENV)) {
        return false;
    }

    rc = SQLAllocHandle(SQL_HANDLE_DBC, henv, &conn);
    if (!OdbcHelper::CheckResult(rc, "SQLAllocHandle failed", conn, SQL_HANDLE_DBC)) {
        return false;
    }

    rc = SQLDriverConnect(conn, nullptr, connection_string, connection_string_len, nullptr, 0, &out_connection_string_len, SQL_DRIVER_NOPROMPT);
    if (!OdbcHelper::CheckResult(rc, "SQLDriverConnect failed", conn, SQL_HANDLE_DBC)) {
        return false;
    }

    return OdbcHelper::CheckLimitlessCluster(conn);
}

bool GetLimitlessInstance(const char *connection_string_c_str, int host_port, const char *service_id_c_str, LimitlessInstance *db_instance) {
    std::vector<HostInfo> hosts;

    {
        std::lock_guard<std::mutex> services_guard(*services_mutex);
        std::shared_ptr<LimitlessMonitorService> service;
        std::string service_id(service_id_c_str);

        if (services.contains(service_id)) {
            service = services[service_id];
            service->depth_counter++;
        } else {
            services[service_id] = std::make_shared<LimitlessMonitorService>();
            service = services[service_id];

            service->depth_counter = 1;
            service->limitless_routers = std::make_shared<std::vector<HostInfo>>();
            service->limitless_routers_mutex = std::make_shared<std::mutex>();
            service->limitless_router_monitor = std::make_shared<LimitlessRouterMonitor>(
                connection_string_c_str,
                host_port,
                DEFAULT_LIMITLESS_MONITOR_INTERVAL,
                service->limitless_routers,
                service->limitless_routers_mutex
            );
        }

        std::lock_guard<std::mutex> limitless_routers_guard(*(service->limitless_routers_mutex));
        hosts = *(service->limitless_routers);
    }

    if (hosts.empty()) {
        return false;
    }

    std::unordered_map<std::string, std::string> properties;
    RoundRobinHostSelector::set_round_robin_weight(hosts, properties);
    HostInfo host_info = round_robin.get_host(hosts, true, properties);

    strncpy(db_instance->server, host_info.get_host().c_str(), db_instance->server_size);

    return true;
}

void StopLimitlessMonitorService(const char *service_id_c_str) {
    std::lock_guard<std::mutex> guard(*services_mutex);
    std::string service_id(service_id_c_str);

    if (services.contains(service_id)) {
        std::shared_ptr<LimitlessMonitorService> service = services[service_id];
        service->depth_counter--;

        if (service->depth_counter == 0) {
            service = nullptr;
            services.erase(service_id); // destroys std::shared_ptr<LimitlessRouterMonitor>
        }
    }
}
