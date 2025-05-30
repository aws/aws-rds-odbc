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
#include "../util/rds_utils.h"
#include "../util/string_helper.h"
#include "limitless_query_helper.h"

static LimitlessMonitorService limitless_monitor_service(std::make_shared<OdbcHelperWrapper>());

LimitlessMonitorService::LimitlessMonitorService(std::shared_ptr<IOdbcHelper> odbc_wrapper) {
    this->odbc_wrapper = std::move(odbc_wrapper);
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
    std::vector<HostInfo> hosts;
    SQLSTR connection_string;

    {
        std::lock_guard<std::mutex> services_guard(*(this->services_mutex));
        if (!this->services.contains(service_id)) {
            LOG(ERROR) << "Attempted to lock and get hosts for non-existent monitor with service ID " << service_id;
            return nullptr;
        }

        std::shared_ptr<LimitlessMonitor> service = this->services[service_id];
        std::lock_guard<std::mutex> limitless_routers_guard(*(service->limitless_routers_mutex));

        if (service->limitless_routers == nullptr || service->limitless_routers->empty())
            return nullptr;

        // copy hosts
        hosts = *(service->limitless_routers);
        connection_string = service->limitless_router_monitor->GetConnectionString();
    }

    std::unordered_map<std::string, std::string> properties;

    try {
        RoundRobinHostSelector::SetRoundRobinWeight(hosts, properties);
        HostInfo host = this->round_robin.GetHost(hosts, true, properties);
        if (this->odbc_wrapper->TestConnectionToServer(connection_string, host.GetHost())) {
            // the round robin host successfully connected
            return std::make_shared<HostInfo>(host);
        }
    } catch (std::runtime_error& error) {
        LOG(INFO) << "Got runtime error while getting round robin host for limitless (trying for highest weight host next): " << error.what();
        // proceed and attempt to connect to highest weight host
    }

    // five retries going by order of least loaded (highest weight)
    for (int i = 0; i < DEFAULT_LIMITLESS_CONNECT_RETRY_ATTEMPTS; i++) {
        try {
            HostInfo host = this->highest_weight.GetHost(hosts, true, properties);

            if (this->odbc_wrapper->TestConnectionToServer(connection_string, host.GetHost())) {
                // the highest weight host successfully connected
                return std::make_shared<HostInfo>(host);
            } else {
                // update this host in hosts list to have down state so it's not selected again
                for (HostInfo& host_in_list : hosts) {
                    if (host_in_list.GetHost() == host.GetHost()) {
                        host_in_list.SetHostState(DOWN);
                    }
                }
            }
        } catch (std::runtime_error &error) {
            // no more hosts
            LOG(INFO) << "Got runtime error while getting highest weight host for limitless (no host found): " << error.what();
            break;
        }
    }

    // no host was successfully connected to
    return nullptr;
}

bool CheckLimitlessCluster(const SQLTCHAR *connection_string_c_str, const char *custom_errmsg_c_str, char *final_errmsg_c_str, size_t final_errmsg_size) {
    SQLHENV henv = SQL_NULL_HANDLE;
    SQLHDBC hdbc = SQL_NULL_HANDLE;

    std::string custom_errmsg = custom_errmsg_c_str == nullptr ? "" : custom_errmsg_c_str;

    if (!OdbcHelper::AllocateHandle(SQL_HANDLE_ENV, nullptr, henv, "Could not allocate environment handle during limitless check") ||
        !OdbcHelper::SetHenvToOdbc3(henv, "Could not set henv to odbc3 during limitless check")) {
        std::string errmsg = StringHelper::MergeStrings("Could not allocate environment handle.", custom_errmsg);
        StringHelper::CopyToCStr(errmsg, final_errmsg_c_str, final_errmsg_size, "Warning: error string is truncated.");

        OdbcHelper::Cleanup(henv, SQL_NULL_HANDLE, SQL_NULL_HANDLE);
        return false;
    }

    if (!OdbcHelper::AllocateHandle(SQL_HANDLE_DBC, henv, hdbc, "ERROR: could not allocate connection handle during limitless check")) {
        std::string errmsg = StringHelper::MergeStrings("Could not allocate connection handle.", custom_errmsg);
        StringHelper::CopyToCStr(errmsg, final_errmsg_c_str, final_errmsg_size, "Warning: error string is truncated.");

        OdbcHelper::Cleanup(henv, hdbc, SQL_NULL_HANDLE);
        return false;
    }

    if (!OdbcHelper::ConnStrConnect(const_cast<SQLTCHAR*>(connection_string_c_str), hdbc)) {
        std::string errmsg = OdbcHelper::MergeDiagRecs(hdbc, SQL_HANDLE_DBC, custom_errmsg);
        StringHelper::CopyToCStr(errmsg, final_errmsg_c_str, final_errmsg_size, "Warning: error string is truncated.");

        OdbcHelper::Cleanup(henv, hdbc, SQL_NULL_HANDLE);
        return false;
    }

    // should this cluster not be a limitless cluster, this function will still return false, but there were no errors
    if (final_errmsg_c_str != nullptr) {
        final_errmsg_c_str[0] = '\0';
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
