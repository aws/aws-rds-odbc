#ifndef CLUSTER_TOPOLOGY_MONITOR_H
#define CLUSTER_TOPOLOGY_MONITOR_H

#include <atomic>
#include <chrono>
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

#include "../host_info.h"
#include "../util/connection_string_helper.h"
#include "../util/logger_wrapper.h"
#include "../util/odbc_helper.h"
#include "../util/sliding_cache_map.h"

#define AS_SQLCHAR(str) const_cast<SQLTCHAR*>(reinterpret_cast<const SQLTCHAR*>(str))
#define AS_CHAR(str) reinterpret_cast<char*>(str)
#define BUFFER_SIZE 1024
#define REPLACE_CHAR "?"

class ClusterTopologyMonitor {
public:
    ClusterTopologyMonitor(const std::string& cluster_id, const std::shared_ptr<SlidingCacheMap<std::string, std::vector<HostInfo>>> topology_map,
        const int default_port, const std::string& conn_str, const std::string& endpoint_template,
        const std::string& topology_query, const std::string& writer_topology_query, const std::string& node_id_query
    );
    ~ClusterTopologyMonitor();

    void set_cluster_id(std::string cluster_id);
    std::vector<HostInfo> force_refresh(const bool verify_writer, const long timeout_ms);
    std::vector<HostInfo> force_refresh(SQLHDBC hdbc, const long timeout_ms);

    void run();

protected:
    std::vector<HostInfo> wait_for_topology_update(long timeout_ms);
    void delay(bool use_high_refresh_rate);
    std::vector<HostInfo> fetch_topology_update_cache(const SQLHDBC hdbc);
    void update_topology_cache(const std::vector<HostInfo>& hosts);
    std::string get_writer_id(const SQLHDBC hdbc) const;
    std::vector<HostInfo> query_topology(const SQLHDBC hdbc) const;
    std::string get_endpoint(SQLTCHAR* node_id) const;
    HostInfo create_host(SQLTCHAR* node_id, bool is_writer, SQLFLOAT cpu_usage, SQLFLOAT replica_lag_ms, SQL_TIMESTAMP_STRUCT update_timestamp) const;
    std::string conn_str_replace_host(std::string conn_in, std::string new_host);

private:
    class NodeMonitoringThread;
    bool in_panic_mode();
    std::vector<HostInfo> open_any_conn_get_hosts();

    std::string cluster_id;
    std::shared_ptr<SlidingCacheMap<std::string, std::vector<HostInfo>>> topology_map;

    int default_port;
    std::string conn_str;

    // Query & Template to be passed in from caller
    // ?.cluster-<Cluster-ID>.<Region>.rds.amazonaws.com
    std::string endpoint_template;
    // SELECT SERVER_ID, CASE WHEN SESSION_ID = 'MASTER_SESSION_ID' THEN TRUE ELSE FALSE END, CPU, COALESCE(REPLICA_LAG_IN_MSEC, 0), LAST_UPDATE_TIMESTAMP FROM aurora_replica_status() WHERE EXTRACT(EPOCH FROM(NOW() - LAST_UPDATE_TIMESTAMP)) <= 300 OR SESSION_ID = 'MASTER_SESSION_ID' OR LAST_UPDATE_TIMESTAMP IS NULL
    std::string topology_query;
    // SELECT SERVER_ID FROM aurora_replica_status() WHERE SESSION_ID = 'MASTER_SESSION_ID' AND SERVER_ID = aurora_db_instance_identifier()
    std::string writer_topology_query;
    // SELECT aurora_db_instance_identifier()
    std::string node_id_query;

    // Track Update Request
    std::atomic<bool> request_update_topology;
    std::mutex request_update_topology_mutex;
    std::condition_variable request_update_topology_cv;

    // Track Topology Updated
    std::mutex topology_updated_mutex;
    std::condition_variable topology_updated;

    std::atomic<std::chrono::steady_clock::time_point> ignore_topology_request_end_ns;
    int ignore_topology_request_ns;
    std::chrono::steady_clock::time_point high_refresh_end_time;
    long high_refresh_rate_ns;
    const std::chrono::seconds high_refresh_rate_after_panic = std::chrono::seconds(30);
    long refresh_rate_ns;
    int default_topology_query_timeout;

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
    SQLHDBC hdbc;

    const int THREAD_SLEEP_MS = 100;
};

#endif // NODE_MONITORING_THREAD_H

#endif // CLUSTER_TOPOLOGY_MONITOR_H
