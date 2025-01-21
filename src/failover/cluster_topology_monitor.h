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


// TODO, support unicode
#ifdef UNICODE
#define SERVER_KEY          "SERVER"
#define FAILOVER_KEY        "ENABLECLUSTERFAILOVER"
#define FAILOVER_DISABLE    "0"
#else
#define SERVER_KEY          "SERVER"
#define FAILOVER_KEY        "ENABLECLUSTERFAILOVER"
#define FAILOVER_DISABLE    "0"
#endif

class ClusterTopologyMonitor {
public:
    ClusterTopologyMonitor(const std::string& cluster_id, const std::shared_ptr<SlidingCacheMap<std::string, std::vector<HostInfo>>> topology_map,
        const std::string& conn_str, std::shared_ptr<IOdbcHelper> odbc_helper,
        std::shared_ptr<ClusterTopologyQueryHelper> query_helper, long ignore_topology_request_ns,
        long high_refresh_rate_ns, long refresh_rate_ns);
    ~ClusterTopologyMonitor();

    void set_cluster_id(const std::string& cluster_id);
    std::vector<HostInfo> force_refresh(const bool verify_writer, const long timeout_ms);
    std::vector<HostInfo> force_refresh(SQLHDBC hdbc, const long timeout_ms);

    void start_monitor();

protected:
    void run();
    std::vector<HostInfo> wait_for_topology_update(long timeout_ms);
    void delay_main_thread(bool use_high_refresh_rate);
    std::vector<HostInfo> fetch_topology_update_cache(const SQLHDBC hdbc);
    void update_topology_cache(const std::vector<HostInfo>& hosts);
    std::string conn_str_replace_host(const std::string& conn_in, const std::string& new_host);

private:
    class NodeMonitoringThread;
    std::shared_ptr<IOdbcHelper> odbc_helper;
    std::shared_ptr<ClusterTopologyQueryHelper> query_helper;
    bool in_panic_mode();
    std::vector<HostInfo> open_any_conn_get_hosts();
    void dbc_clean_up(std::shared_ptr<SQLHDBC> dbc);

    bool handle_panic_mode();
    bool handle_regular_mode();
    void handle_ignore_topology_timing();
    void init_node_monitors();
    bool get_possible_writer_conn();

    // Topology Tracking
    std::string cluster_id;
    std::string conn_str;
    // SlidingCacheMap internally is thread safe
    std::shared_ptr<SlidingCacheMap<std::string, std::vector<HostInfo>>> topology_map;

    // Track Update Request
    std::atomic<bool> request_update_topology;
    std::mutex request_update_topology_mutex;
    std::condition_variable request_update_topology_cv;

    // Track Topology Updated
    std::mutex topology_updated_mutex;
    std::condition_variable topology_updated;

    std::atomic<std::chrono::steady_clock::time_point> ignore_topology_request_end_ns;
    long ignore_topology_request_ns;
    std::chrono::steady_clock::time_point high_refresh_end_time;
    long high_refresh_rate_ns;
    const std::chrono::seconds high_refresh_rate_after_panic = std::chrono::seconds(30);
    long refresh_rate_ns;

    // Main Thread
    std::shared_ptr<std::thread> monitoring_thread; 
    std::atomic<bool> is_running;
    // Children / Node Threads
    std::map<std::string, std::shared_ptr<NodeMonitoringThread>> node_monitoring_threads;
    std::atomic<bool> node_threads_stop;

    // Children Thread Connections & Host Info
    std::shared_ptr<SQLHDBC> node_threads_writer_hdbc;
    std::shared_ptr<HostInfo> node_threads_writer_host_info;
    std::shared_ptr<SQLHDBC> node_threads_reader_hdbc;
    std::shared_ptr<std::vector<HostInfo>> node_threads_latest_topology;

    std::mutex node_threads_writer_hdbc_mutex;
    std::mutex node_threads_writer_host_info_mutex;
    std::mutex node_threads_reader_hdbc_mutex;
    std::mutex node_threads_latest_topology_mutex;

    // TODO, review if these can be done without mutex/atomics
    // There should be only at most 1 thread interacting with these
    // Connection Information for main thread
    std::atomic<bool> is_writer_connection;
    SQLHENV henv;
    std::mutex hdbc_mutex;
    std::shared_ptr<SQLHDBC> main_hdbc;
    std::shared_ptr<HostInfo> main_writer_host_info;
};

#ifndef NODE_MONITORING_THREAD_H
#define NODE_MONITORING_THREAD_H

class ClusterTopologyMonitor::NodeMonitoringThread {
public:
    NodeMonitoringThread(ClusterTopologyMonitor* monitor, std::shared_ptr<HostInfo> host_info,
        std::shared_ptr<HostInfo> writer_host_info, std::string conn_str);
    ~NodeMonitoringThread();

private:
    void run();
    void reader_thread_fetch_topology();

    ClusterTopologyMonitor* main_monitor;
    std::shared_ptr<HostInfo> host_info;
    std::shared_ptr<HostInfo> writer_host_info;
    std::string conn_str;
    bool writer_changed = false;    
    std::shared_ptr<std::thread> node_thread;
    SQLHDBC hdbc = SQL_NULL_HDBC;

    const int THREAD_SLEEP_MS = 100;
};

#endif // NODE_MONITORING_THREAD_H

#endif // CLUSTER_TOPOLOGY_MONITOR_H
