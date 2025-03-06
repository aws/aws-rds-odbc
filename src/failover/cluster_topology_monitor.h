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

#ifndef CLUSTER_TOPOLOGY_MONITOR_H
#define CLUSTER_TOPOLOGY_MONITOR_H

#include <atomic>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <map>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

// ODBC APIs
#ifdef WIN32
    #include <windows.h>
#endif
#include <sql.h>
#include <sqlext.h>
#include <sqltypes.h>

#include "cluster_topology_query_helper.h"

#include "../host_info.h"
#include "../util/connection_string_helper.h"
#include "../util/logger_wrapper.h"
#include "../util/odbc_helper.h"
#include "../util/sliding_cache_map.h"

#ifdef UNICODE
#define SERVER_KEY          L"SERVER"
#define FAILOVER_KEY        L"ENABLECLUSTERFAILOVER"
#define FAILOVER_DISABLE    L"0"
#else
#define SERVER_KEY          "SERVER"
#define FAILOVER_KEY        "ENABLECLUSTERFAILOVER"
#define FAILOVER_DISABLE    "0"
#endif

class ClusterTopologyMonitor {
public:
    ClusterTopologyMonitor(const std::string& cluster_id, const std::shared_ptr<SlidingCacheMap<std::string, std::vector<HostInfo>>>& topology_map,
        const SQLTCHAR* conn_cstr, const std::shared_ptr<IOdbcHelper>& odbc_helper,
        const std::shared_ptr<ClusterTopologyQueryHelper>& query_helper, uint64_t ignore_topology_request_ns,
        uint64_t high_refresh_rate_ns, uint64_t refresh_rate_ns);
    ~ClusterTopologyMonitor();

    void SetClusterId(const std::string& cluster_id);
    std::vector<HostInfo> ForceRefresh(bool verify_writer, uint64_t timeout_ms);
    std::vector<HostInfo> ForceRefresh(SQLHDBC hdbc, uint64_t timeout_ms);

    void StartMonitor();

protected:
    void run();
    std::vector<HostInfo> WaitForTopologyUpdate(uint64_t timeout_ms);
    void DelayMainThread(bool use_high_refresh_rate);
    std::vector<HostInfo> FetchTopologyUpdateCache(SQLHDBC hdbc);
    void UpdateTopologyCache(const std::vector<HostInfo>& hosts);
    #ifdef UNICODE
    std::wstring ConnForHost(const std::string& new_host);
    #else
    std::string ConnForHost(const std::string& new_host);
    #endif

private:
    class NodeMonitoringThread;
    std::shared_ptr<IOdbcHelper> odbc_helper_;
    std::shared_ptr<ClusterTopologyQueryHelper> query_helper_;
    bool in_panic_mode();
    std::vector<HostInfo> open_any_conn_get_hosts();
    static void dbc_clean_up(std::shared_ptr<SQLHDBC>& dbc);

    bool handle_panic_mode();
    bool handle_regular_mode();
    void handle_ignore_topology_timing();
    void init_node_monitors();
    bool get_possible_writer_conn();

    // Topology Tracking
    std::string cluster_id_;
    #ifdef UNICODE
        std::wstring conn_str_;
    #else
        std::string conn_str_;
    #endif

    // SlidingCacheMap internally is thread safe
    std::shared_ptr<SlidingCacheMap<std::string, std::vector<HostInfo>>> topology_map_;

    // Track Update Request
    std::atomic<bool> request_update_topology_;
    std::mutex request_update_topology_mutex_;
    std::condition_variable request_update_topology_cv_;
    const uint64_t TOPOLOGY_REQUEST_WAIT_MS = 50;

    // Track Topology Updated
    std::mutex topology_updated_mutex_;
    std::condition_variable topology_updated_;
    const uint64_t TOPOLOGY_UPDATE_WAIT_MS = 1000;

    std::atomic<std::chrono::steady_clock::time_point> ignore_topology_request_end_ns_;
    uint64_t ignore_topology_request_ns_;
    std::chrono::steady_clock::time_point high_refresh_end_time_;
    uint64_t high_refresh_rate_ns_;
    const std::chrono::seconds high_refresh_rate_after_panic_ = std::chrono::seconds(30);
    uint64_t refresh_rate_ns_;

    // Main Thread
    std::shared_ptr<std::thread> monitoring_thread_; 
    std::atomic<bool> is_running_;
    // Children / Node Threads
    std::map<std::string, std::shared_ptr<NodeMonitoringThread>> node_monitoring_threads_;
    std::atomic<bool> node_threads_stop_;

    // Children Thread Connections & Host Info
    std::shared_ptr<SQLHDBC> node_threads_writer_hdbc_;
    std::shared_ptr<HostInfo> node_threads_writer_host_info_;
    std::shared_ptr<SQLHDBC> node_threads_reader_hdbc_;
    std::shared_ptr<std::vector<HostInfo>> node_threads_latest_topology_;

    std::mutex node_threads_writer_hdbc_mutex_;
    std::mutex node_threads_writer_host_info_mutex_;
    std::mutex node_threads_reader_hdbc_mutex_;
    std::mutex node_threads_latest_topology_mutex_;

    // TODO(yuenhcol), review if these can be done without mutex/atomics
    // There should be only at most 1 thread interacting with these
    // Connection Information for main thread
    std::atomic<bool> is_writer_connection_;
    SQLHENV henv_;
    std::mutex hdbc_mutex_;
    std::shared_ptr<SQLHDBC> main_hdbc_;
    std::shared_ptr<HostInfo> main_writer_host_info_;
};

class ClusterTopologyMonitor::NodeMonitoringThread {
public:
    NodeMonitoringThread(ClusterTopologyMonitor* monitor, const std::shared_ptr<HostInfo>& host_info,
        const std::shared_ptr<HostInfo>& writer_host_info);
    ~NodeMonitoringThread();

private:
    void run();
    void handle_reconnect(SQLTCHAR* conn_cstr);
    void handle_writer_conn();
    void handle_reader_conn(bool& thread_update_topology);
    void reader_thread_fetch_topology();

    ClusterTopologyMonitor* main_monitor_;
    std::shared_ptr<HostInfo> host_info_;
    std::shared_ptr<HostInfo> writer_host_info_;
    bool writer_changed_ = false;    
    std::shared_ptr<std::thread> node_thread_;
    SQLHDBC hdbc_ = SQL_NULL_HDBC;

    const uint64_t THREAD_SLEEP_MS_ = 100;
};

#endif // CLUSTER_TOPOLOGY_MONITOR_H
