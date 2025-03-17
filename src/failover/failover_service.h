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

#ifndef FAILOVER_SERVICE_H
#define FAILOVER_SERVICE_H

#ifdef __cplusplus

#include <atomic>
#include <iostream>
#include <map>
#include <memory>
#include <string>

#include "cluster_topology_monitor.h"

#include "../dialect/dialect.h"
#include "../dialect/dialect_aurora_postgres.h"
#include "../host_selector/highest_weight_host_selector.h"
#include "../host_selector/host_selector.h"
#include "../host_selector/random_host_selector.h"
#include "../host_selector/round_robin_host_selector.h"
#include "../util/connection_string_helper.h"
#include "../util/connection_string_keys.h"
#include "../util/odbc_helper.h"
#include "../util/sliding_cache_map.h"

#define BUFFER_SIZE 1024
#define MAX_CLUSTER_ID_LEN 1024

extern "C" {
#endif

#define FOREACH_FAILOVER_MODE(FAILOVER_MODE)    \
    FAILOVER_MODE(UNKNOWN_FAILOVER_MODE)        \
    FAILOVER_MODE(STRICT_READER)                \
    FAILOVER_MODE(STRICT_WRITER)                \
    FAILOVER_MODE(READER_OR_WRITER)             \

#define FOR_EACH_STRATEGY(STRATEGY) \
    STRATEGY(UNKNOWN_STRATEGY)      \
    STRATEGY(RANDOM)                \
    STRATEGY(ROUND_ROBIN)           \
    STRATEGY(HIGHEST_WEIGHT)        \

#define GENERATE_ENUM(ENUM) ENUM,
#define GENERATE_STRING(STRING) #STRING,
#define GENERATE_MAPPING(MODE) { MODE, #MODE },

typedef enum {
    FOREACH_FAILOVER_MODE(GENERATE_ENUM)
} FailoverMode;

typedef enum {
    FOR_EACH_STRATEGY(GENERATE_ENUM)
} ReaderHostSelectorStrategy;

static const char* failover_mode_str[] = {
    FOREACH_FAILOVER_MODE(GENERATE_STRING)
};

static const char* strategy_str[] = {
    FOR_EACH_STRATEGY(GENERATE_STRING)
};

static const struct {
    FailoverMode mode;
    const char* mode_str;
} FAILOVER_MODE_MAPPING[] = {
    FOREACH_FAILOVER_MODE(GENERATE_MAPPING)
};

static const struct {
    ReaderHostSelectorStrategy mode;
    const char* mode_str;
} READER_HOST_SELECTOR_STRATEGIES_MAPPING[] = {
    FOR_EACH_STRATEGY(GENERATE_MAPPING)
};

typedef enum {
    AURORA_POSTGRES
} DatabaseDialect;

typedef struct {
    bool connection_changed;
    SQLHDBC hdbc;
} FailoverResult;

/**
 * Increments the reference count or starts a new Failover Service for a given cluster/service ID.
 * 
 * @param service_id_c_str an identifier used to track the reference count of the failover service
 * @param dialect enum value for different database dialects for queries and default ports
 * @param conn_cstr connection string to specifiy the settings
 * @return true if a service is either incremented or started
 */
bool StartFailoverService(char* service_id_c_str, DatabaseDialect dialect, const SQLTCHAR* conn_cstr);

/**
 * Decrements the reference count a Failover Service for a given cluster/service ID.
 * If the reference count drops below 0 and is not currently undergoing failover,
 * the service will be cleaned up
 * 
 * @param service_id_c_str an identifier used to track the reference count of the failover service
 */
void StopFailoverService(const char* service_id_c_str);

/**
 * Given a service ID and the current connection's SQL State,
 * if the SQL State is communication link error, Failover will attempt to update topology and
 * and return a new HDBC connection
 * 
 * @param service_id_c_str an identifier used to track the reference count of the failover service
 * @param sql_state the SQL State of the connection
 * @return a FailoverResult object indicating whether the connection has been established, and if so the new connection.
 */
FailoverResult FailoverConnection(const char* service_id_c_str, const char* sql_state);

#ifdef __cplusplus
}

class FailoverService {
public:
    static const uint32_t DEFAULT_IGNORE_TOPOLOGY_REQUEST_MS =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::seconds(30)).count();
    static const uint32_t DEFAULT_HIGH_REFRESH_RATE_MS =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::seconds(10)).count();
    static const uint32_t DEFAULT_REFRESH_RATE_MS =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::seconds(30)).count();
    static const uint32_t DEFAULT_FAILOVER_TIMEOUT_MS =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::seconds(30)).count();

#ifdef UNICODE
    FailoverService(const std::string& host, const std::string& cluster_id, std::shared_ptr<Dialect> dialect,
        std::shared_ptr<std::map<std::wstring, std::wstring>> conn_info,
        std::shared_ptr<SlidingCacheMap<std::string, std::vector<HostInfo>>> topology_map,
        const std::shared_ptr<ClusterTopologyMonitor>& topology_monitor,
        const std::shared_ptr<IOdbcHelper>& odbc_helper);
#else
    FailoverService(const std::string& host, const std::string& cluster_id, std::shared_ptr<Dialect> dialect,
        std::shared_ptr<std::map<std::string, std::string>> conn_info,
        std::shared_ptr<SlidingCacheMap<std::string, std::vector<HostInfo>>> topology_map,
        const std::shared_ptr<ClusterTopologyMonitor>& topology_monitor,
        const std::shared_ptr<IOdbcHelper>& odbc_helper);
#endif
    ~FailoverService();

    bool Failover(SQLHDBC hdbc, const char* sql_state);
    HostInfo GetCurrentHost();

private:
    static const int MAX_STATE_LENGTH = 32;
    static const int MAX_MSG_LENGTH = 1024;
    static bool check_should_failover(const char* sql_state);
    static void remove_candidate(const std::string& host, std::vector<HostInfo>& candidates);
    bool failover_reader(SQLHDBC hdbc);
    bool failover_writer(SQLHDBC hdbc);
    bool connect_to_host(SQLHDBC hdbc, const std::string& host_string);
    bool is_connected_to_reader(SQLHDBC hdbc);
    bool is_connected_to_writer(SQLHDBC hdbc);
    void init_failover_mode(const std::string& host);
    std::shared_ptr<HostSelector> get_reader_host_selector() const;

    HostInfo curr_host_;
    std::string cluster_id_;
    std::shared_ptr<Dialect> dialect_;
    #ifdef UNICODE
    std::shared_ptr<std::map<std::wstring, std::wstring>> conn_info_;
    #else
    std::shared_ptr<std::map<std::string, std::string>> conn_info_;
    #endif
    std::shared_ptr<HostSelector> host_selector_;
    std::shared_ptr<SlidingCacheMap<std::string, std::vector<HostInfo>>> topology_map_;
    std::shared_ptr<ClusterTopologyMonitor> topology_monitor_;
    std::shared_ptr<IOdbcHelper> odbc_helper_;
    FailoverMode failover_mode_ = UNKNOWN_FAILOVER_MODE;
    uint32_t failover_timeout_;
};

typedef struct FailoverServiceTracker {
    ~FailoverServiceTracker() {
        this->reference_count = 0;
        this->failover_inprogress = 0;
        this->service = nullptr;
    };
    std::atomic<int> reference_count = 0;
    std::atomic<int> failover_inprogress = 0;
    std::shared_ptr<FailoverService> service = nullptr;
};

#endif

#endif // FAILOVER_SERVICE_H
