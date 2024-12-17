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

#include <cstdio>
#include <cstring>

#include "../util/logger_wrapper.h"
#include "limitless_monitor_service.h"
#include "odbc_helper.h"

static LimitlessMonitorService limitless_monitor_service;

LimitlessMonitorService::LimitlessMonitorService() {
    this->services_mutex = std::make_shared<std::mutex>();
}

LimitlessMonitorService::~LimitlessMonitorService() {
    std::lock_guard<std::mutex> services_guard(*(this->services_mutex));
    this->services.clear(); // destroys everything
}

bool LimitlessMonitorService::CheckService(std::string service_id) {
    std::lock_guard<std::mutex> services_guard(*(this->services_mutex));
    return this->services.contains(service_id);
}

void LimitlessMonitorService::NewService(
    std::string service_id,
    const char *connection_string_c_str,
    int host_port,
    std::shared_ptr<LimitlessRouterMonitor> limitless_router_monitor
) {
    printf("59\n");
    std::lock_guard<std::mutex> services_guard(*(this->services_mutex));
    
    printf("62\n");
    std::shared_ptr<LimitlessMonitor> service = std::make_shared<LimitlessMonitor>();
    service->reference_counter = 0; // caller should call IncrementReferenceCounter for this service ID
    service->limitless_routers = std::make_shared<std::vector<HostInfo>>();
    service->limitless_routers_mutex = std::make_shared<std::mutex>();
    service->limitless_router_monitor = limitless_router_monitor;
    this->services[service_id] = service;

    printf("70\n");
    // start monitoring; will block until first set of limitless routers is retrieved or until an error occurs (leaving limitless_routers is empty)
    limitless_router_monitor->Open(connection_string_c_str, host_port, DEFAULT_LIMITLESS_MONITOR_INTERVAL, service->limitless_routers, service->limitless_routers_mutex);
    LOG(INFO) << "Limitless Monitor Service: started monitoring with service ID " << service_id;
}

void LimitlessMonitorService::IncrementReferenceCounter(std::string service_id) {
    printf("77\n");
    std::lock_guard<std::mutex> services_guard(*(this->services_mutex));
    std::shared_ptr<LimitlessMonitor> service = this->services[service_id];
    service->reference_counter++;
}

void LimitlessMonitorService::DecrementReferenceCounter(std::string service_id) {
    printf("84\n");
    std::lock_guard<std::mutex> services_guard(*(this->services_mutex));
    std::shared_ptr<LimitlessMonitor> service = this->services[service_id];
    
    if (service->reference_counter > 0) {
        service->reference_counter--;
    } else {
        LOG(ERROR) << "Limitless Monitor Service: monitor with service ID " << service_id << " has improper reference counter";
    }

    if (service->reference_counter == 0) {
        service = nullptr;
        services.erase(service_id);
        LOG(INFO) << "Limitless Monitor Service: stopped monitoring with service ID " << service_id;
    }
}

std::shared_ptr<HostInfo> LimitlessMonitorService::GetHostInfo(std::string service_id) {
    std::lock_guard<std::mutex> services_guard(*(this->services_mutex));
    std::shared_ptr<LimitlessMonitor> service = this->services[service_id];

    std::lock_guard<std::mutex> limitless_routers_guard(*(service->limitless_routers_mutex));
    std::vector<HostInfo> hosts = *(service->limitless_routers);
    if (hosts.empty()) {
        return nullptr;
    }

    std::unordered_map<std::string, std::string> properties;
    RoundRobinHostSelector::set_round_robin_weight(hosts, properties);
    std::shared_ptr<HostInfo> host = std::make_shared<HostInfo>(round_robin.get_host(hosts, true, properties));
    return host;
}

bool CheckLimitlessCluster(const char *connection_string_c_str) {
    SQLHENV henv = SQL_NULL_HANDLE;
    SQLHDBC conn = SQL_NULL_HANDLE;
    auto *connection_string = const_cast<SQLCHAR *>(reinterpret_cast<const SQLCHAR *>(connection_string_c_str));
    SQLSMALLINT connection_string_len = strlen(connection_string_c_str);
    SQLSMALLINT out_connection_string_len; // unused

    SQLRETURN rc = SQLAllocHandle(SQL_HANDLE_ENV, nullptr, &henv);
    if (!OdbcHelper::CheckResult(rc, "Limitless Monitor Service: SQLAllocHandle failed in CheckLimitlessCluster", henv, SQL_HANDLE_ENV)) {
        OdbcHelper::Cleanup(henv, NULL, NULL);
        return false;
    }

    rc = SQLAllocHandle(SQL_HANDLE_DBC, henv, &conn);
    if (!OdbcHelper::CheckResult(rc, "Limitless Monitor Service: SQLAllocHandle failed in CheckLimitlessCluster", conn, SQL_HANDLE_DBC)) {
        OdbcHelper::Cleanup(henv, conn, NULL);
        return false;
    }

    rc = SQLDriverConnect(conn, nullptr, connection_string, connection_string_len, nullptr, 0, &out_connection_string_len, SQL_DRIVER_NOPROMPT);
    if (!OdbcHelper::CheckResult(rc, "Limitless Monitor Service: SQLDriverConnect failed in CheckLimitlessCluster", conn, SQL_HANDLE_DBC)) {
        OdbcHelper::Cleanup(henv, conn, NULL);
        return false;
    }

    bool result = OdbcHelper::CheckLimitlessCluster(conn);
    OdbcHelper::Cleanup(henv, conn, NULL);
    return result;
}

bool GetLimitlessInstance(const char *connection_string_c_str, int host_port, const char *service_id_c_str, LimitlessInstance *db_instance) {
    std::string service_id(service_id_c_str);

    if (!limitless_monitor_service.CheckService(service_id)) {
        limitless_monitor_service.NewService(service_id, connection_string_c_str, host_port, std::make_shared<LimitlessRouterMonitor>());
    }

    limitless_monitor_service.IncrementReferenceCounter(service_id);
    std::shared_ptr<HostInfo> host = limitless_monitor_service.GetHostInfo(service_id);
    if (host == nullptr) {
        return false;
    }

    strncpy(db_instance->server, host->get_host().c_str(), db_instance->server_size);
    return true;
}

void StopLimitlessMonitorService(const char *service_id_c_str) {
    std::string service_id(service_id_c_str);
    limitless_monitor_service.DecrementReferenceCounter(service_id);
}
