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

#include <cstdio>
#include <cstring>

#include "../util/logger_wrapper.h"
#include "connection_string_helper.h"
#include "limitless_monitor_service.h"

#include "limitless_query_helper.h"
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
    ConnectionStringHelper::ParseConnectionStringW(reinterpret_cast<const wchar_t *>(connection_string_c_str), connection_string_map);
    #else
    std::map<std::string, std::string> connection_string_map;
    ConnectionStringHelper::ParseConnectionString(reinterpret_cast<const char *>(connection_string_c_str), connection_string_map);
    #endif

    bool block_and_query_immediately = true;

    auto it = connection_string_map.find(LIMITLESS_MODE_KEY);
    if (it != connection_string_map.end()) {
        #ifdef UNICODE
        std::wstring value = connection_string_map[LIMITLESS_MODE_KEY];
        #else
        std::string value = connection_string_map[LIMITLESS_MODE_KEY];
        #endif
        if (value == LIMITLESS_MODE_VALUE_LAZY) {
            block_and_query_immediately = false;
        }
    }

    unsigned int limitless_monitor_interval_ms = DEFAULT_LIMITLESS_MONITOR_INTERVAL_MS; // incase the field is unset

    it = connection_string_map.find(LIMITLESS_MONITOR_INTERVAL_MS_KEY);
    if (it != connection_string_map.end()) {
        #ifdef UNICODE
        std::wstring value = connection_string_map[LIMITLESS_MONITOR_INTERVAL_MS_KEY];
        #else
        std::string value = connection_string_map[LIMITLESS_MONITOR_INTERVAL_MS_KEY];
        #endif
        limitless_monitor_interval_ms = std::stoi(value);
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
        limitless_monitor_interval_ms,
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
    bool result = LimitlessQueryHelper::CheckLimitlessCluster(hdbc);
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
