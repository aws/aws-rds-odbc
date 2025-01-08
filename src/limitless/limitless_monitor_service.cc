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
#include "connection_string_helper.h"
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

bool LimitlessMonitorService::CheckService(const std::string& service_id) {
    std::lock_guard<std::mutex> services_guard(*(this->services_mutex));
    return this->services.contains(service_id);
}

bool LimitlessMonitorService::NewService(
    const std::string& service_id,
    const SQLTCHAR *connection_string_c_str,
    int host_port,
    std::shared_ptr<LimitlessRouterMonitor> limitless_router_monitor
) {
    std::lock_guard<std::mutex> services_guard(*(this->services_mutex));
    if (this->services.contains(service_id)) {
        LOG(ERROR) << "Attempted to recreate existing monitor with service ID " << service_id;
        return false;
    }

    // parse the connection string to extract useful limitless information
    #ifdef UNICODE
    std::map<std::wstring, std::wstring> connection_string_map;
    ConnectionStringHelper::ParseConnectionString(reinterpret_cast<const wchar_t *>(connection_string_c_str), connection_string_map);
    #else
    std::map<std::string, std::string> connection_string_map;
    ConnectionStringHelper::ParseConnectionString(reinterpret_cast<const char *>(connection_string_c_str), connection_string_map);
    #endif

    bool block_and_query_immediately = true;

    if (connection_string_map.contains(LIMITLESS_MODE_KEY)) {
        #ifdef UNICODE
        std::wstring limitless_mode = connection_string_map[LIMITLESS_MODE_KEY];
        #else
        std::string limitless_mode = connection_string_map[LIMITLESS_MODE_KEY];
        #endif
        if (limitless_mode == LIMITLESS_MODE_VALUE_LAZY) {
            block_and_query_immediately = false;
        }
    }

    // ensure that the owning scope of the shared pointer is inside the map
    this->services[service_id] = std::make_shared<LimitlessMonitor>();

    std::shared_ptr<LimitlessMonitor> service = this->services[service_id];
    service->reference_counter = 1;
    service->limitless_routers = std::make_shared<std::vector<HostInfo>>();
    service->limitless_routers_mutex = std::make_shared<std::mutex>();
    service->limitless_router_monitor = std::move(limitless_router_monitor);
    // limitless_router_monitor is now nullptr

    // start monitoring; this will block until the first set of limitless routers
    // is retrieved or an error occurs if block_and_query_immediately is true
    service->limitless_router_monitor->Open(
        block_and_query_immediately,
        connection_string_c_str,
        host_port,
        DEFAULT_LIMITLESS_MONITOR_INTERVAL_MS,
        service->limitless_routers,
        service->limitless_routers_mutex
    );
    LOG(INFO) << "Started monitoring with service ID " << service_id;

    return true;
}

void LimitlessMonitorService::IncrementReferenceCounter(const std::string& service_id) {
    std::lock_guard<std::mutex> services_guard(*(this->services_mutex));
    if (!this->services.contains(service_id)) {
        LOG(ERROR) << "Attempted to increment reference counter for non-existent monitor with service ID " << service_id;
        return;
    }

    std::shared_ptr<LimitlessMonitor> service = this->services[service_id];
    service->reference_counter++;
}

void LimitlessMonitorService::DecrementReferenceCounter(const std::string& service_id) {
    std::lock_guard<std::mutex> services_guard(*(this->services_mutex));
    if (!this->services.contains(service_id)) {
        LOG(ERROR) << "Attempted to decrement reference counter for non-existent monitor with service ID " << service_id;
        return;
    }

    std::shared_ptr<LimitlessMonitor> service = this->services[service_id];

    if (service->reference_counter > 0) {
        service->reference_counter--;
    } else {
        LOG(ERROR) << "Monitor with service ID " << service_id << " has improper reference counter";
    }

    if (service->reference_counter == 0) {
        service = nullptr;
        services.erase(service_id);
        LOG(INFO) << "Stopped monitoring with service ID " << service_id;
    }
}

std::shared_ptr<HostInfo> LimitlessMonitorService::GetHostInfo(const std::string& service_id) {
    std::lock_guard<std::mutex> services_guard(*(this->services_mutex));
    if (!this->services.contains(service_id)) {
        LOG(ERROR) << "Attempted to get host info for non-existent monitor with service ID " << service_id;
        return nullptr;
    }

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

bool CheckLimitlessCluster(SQLHDBC hdbc) {
    bool result = OdbcHelper::CheckLimitlessCluster(hdbc);
    return result;
}

bool GetLimitlessInstance(const SQLTCHAR *connection_string_c_str, int host_port, const char *service_id_c_str, const LimitlessInstance *db_instance) {
    std::string service_id(service_id_c_str);

    if (!limitless_monitor_service.CheckService(service_id)) {
        if (!limitless_monitor_service.NewService(service_id, connection_string_c_str, host_port, std::make_shared<LimitlessRouterMonitor>())) {
            return false;
        }
    } else {
        limitless_monitor_service.IncrementReferenceCounter(service_id);
    }

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
