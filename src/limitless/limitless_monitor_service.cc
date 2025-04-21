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

#include "limitless_monitor_service.h"

#ifdef WIN32
#include <windows.h>
#endif

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <glog/logging.h>

#include "../util/connection_string_helper.h"
#include "../util/connection_string_keys.h"
#include "../util/logger_wrapper.h"
#include "../util/odbc_helper.h"
#include "../util/string_helper.h"
#include "limitless_query_helper.h"
#include "rds_utils.h"

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
    std::string& service_id,
    const SQLTCHAR *connection_string_c_str,
    int host_port,
    std::shared_ptr<LimitlessRouterMonitor> limitless_router_monitor
) {
    std::lock_guard<std::mutex> services_guard(*(this->services_mutex));

    // parse the connection string to extract useful limitless information
    std::map<SQLSTR, SQLSTR> connection_string_map;
    SQLSTR conn_str = StringHelper::ToSQLSTR(connection_string_c_str);
    ConnectionStringHelper::ParseConnectionString(conn_str, connection_string_map);

    if (service_id.empty()) {
        auto it = connection_string_map.find(SERVER_HOST_KEY);
        if (it != connection_string_map.end()) {
            std::string host = StringHelper::ToString(connection_string_map[SERVER_HOST_KEY]);
            service_id = RdsUtils::GetRdsClusterId(host);

            if (service_id.empty()) {
                service_id = std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
                LOG(INFO) << "No service ID provided and could not parse service ID from host: " << host << ". Generated random service ID: " << service_id;
            }
        }
    }

    if (this->services.contains(service_id)) {
        LOG(ERROR) << "Attempted to recreate existing monitor with service ID " << service_id;
        return false;
    }

    bool block_and_query_immediately = true;

    auto it = connection_string_map.find(LIMITLESS_MODE_KEY);
    if (it != connection_string_map.end()) {
        SQLSTR value = connection_string_map[LIMITLESS_MODE_KEY];
        if (value == LIMITLESS_MODE_VALUE_LAZY) {
            block_and_query_immediately = false;
        }
    }

    unsigned int limitless_monitor_interval_ms = DEFAULT_LIMITLESS_MONITOR_INTERVAL_MS; // incase the field is unset

    it = connection_string_map.find(LIMITLESS_MONITOR_INTERVAL_MS_KEY);
    if (it != connection_string_map.end()) {
        SQLSTR value = connection_string_map[LIMITLESS_MONITOR_INTERVAL_MS_KEY];
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
    RoundRobinHostSelector::SetRoundRobinWeight(hosts, properties);
    std::shared_ptr<HostInfo> host = std::make_shared<HostInfo>(round_robin.GetHost(hosts, true, properties));
    return host;
}

bool CheckLimitlessCluster(const SQLTCHAR *connection_string_c_str, char *errmsg_out_c_str, size_t errmsg_out_size, char *custom_errmsg_c_str) {
    SQLHENV henv = SQL_NULL_HANDLE;
    SQLHDBC hdbc = SQL_NULL_HANDLE;

    if (!OdbcHelper::AllocateHandle(SQL_HANDLE_ENV, nullptr, henv, "Could not allocate environment handle during limitless check") ||
        !OdbcHelper::SetHenvToOdbc3(henv, "Could not set henv to odbc3 during limitless check")) {
        OdbcHelper::Cleanup(henv, SQL_NULL_HANDLE, SQL_NULL_HANDLE);
        return false;
    }

    if (!OdbcHelper::AllocateHandle(SQL_HANDLE_DBC, henv, hdbc, "ERROR: could not allocate connection handle during limitless check")) {
        OdbcHelper::Cleanup(henv, hdbc, SQL_NULL_HANDLE);
        return false;
    }

    if (!OdbcHelper::ConnStrConnect(const_cast<SQLTCHAR*>(connection_string_c_str), hdbc)) {
        if (errmsg_out_c_str != nullptr) {
            std::string custom_errmsg = custom_errmsg_c_str == nullptr ? "" : custom_errmsg_c_str;
            std::string errmsg = OdbcHelper::MergeDiagRecs(hdbc, custom_errmsg);
            strncpy(errmsg_out_c_str, errmsg.c_str(), errmsg_out_size);
        }

        OdbcHelper::Cleanup(henv, hdbc, SQL_NULL_HANDLE);
        return false;
    }

    if (errmsg_out_c_str != nullptr) {
        errmsg_out_c_str[0] = '\0';
    }

    bool is_limitess_cluster = LimitlessQueryHelper::CheckLimitlessCluster(hdbc);
    OdbcHelper::Cleanup(henv, hdbc, SQL_NULL_HANDLE);
    return is_limitess_cluster;
}

bool GetLimitlessInstance(const SQLTCHAR *connection_string_c_str, int host_port, char *service_id_c_str, size_t service_id_size, const LimitlessInstance *db_instance) {
    std::string service_id(service_id_c_str);

    if (service_id.empty() || !limitless_monitor_service.CheckService(service_id)) {
        bool service_id_was_empty = service_id.empty();
        if (!limitless_monitor_service.NewService(service_id, connection_string_c_str, host_port, std::make_shared<LimitlessRouterMonitor>())) {
            return false;
        }
        // overwrite the provided service ID if it was overwritten
        if (service_id_was_empty) {
            strncpy(service_id_c_str, service_id.c_str(), service_id_size);
        }
    } else {
        limitless_monitor_service.IncrementReferenceCounter(service_id);
    }

    std::shared_ptr<HostInfo> host = limitless_monitor_service.GetHostInfo(service_id);
    if (host == nullptr) {
        return false;
    }

    strncpy(db_instance->server, host->GetHost().c_str(), db_instance->server_size);
    return true;
}

void StopLimitlessMonitorService(const char *service_id_c_str) {
    std::string service_id(service_id_c_str);
    limitless_monitor_service.DecrementReferenceCounter(service_id);
}
