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

#include "cluster_topology_monitor.h"

ClusterTopologyMonitor::ClusterTopologyMonitor(
        const std::string& cluster_id,
        const std::shared_ptr<SlidingCacheMap<std::string, std::vector<HostInfo>>>& topology_map,
        const SQLTCHAR* conn_cstr,
        const std::shared_ptr<IOdbcHelper>& odbc_helper,
        const std::shared_ptr<ClusterTopologyQueryHelper>& query_helper,
        uint64_t ignore_topology_request_ns,
        uint64_t high_refresh_rate_ns,
        uint64_t refresh_rate_ns):
        cluster_id{ cluster_id },
        topology_map { topology_map },
        odbc_helper { std::move(odbc_helper) },
        query_helper { std::move(query_helper) },
        ignore_topology_request_ns { ignore_topology_request_ns },
        high_refresh_rate_ns { high_refresh_rate_ns },
        refresh_rate_ns { refresh_rate_ns } {

    if (!this->odbc_helper->CheckResult(SQLAllocHandle(SQL_HANDLE_ENV, nullptr, &henv),
        std::string("Cluster Topology Monitor unable to allocate environment handle for: ") + cluster_id,
            henv, SQL_HANDLE_ENV)) {
        throw std::runtime_error(std::string("Cluster Topology Monitor unable to allocate HENV for ClusterId: ") + cluster_id);
    }
    #ifdef UNICODE
    conn_str = std::wstring(reinterpret_cast<const wchar_t*>(conn_cstr));
    #else
    conn_str = std::string(reinterpret_cast<const char*>(conn_cstr));
    #endif
}

ClusterTopologyMonitor::~ClusterTopologyMonitor() {
    is_running.store(false);
    node_threads_stop.store(true);

    // Notify node threads if they are stuck waiting
    {
        std::lock_guard<std::mutex> lock(request_update_topology_mutex);
        request_update_topology.store(true);
    }
    request_update_topology_cv.notify_all();
    node_monitoring_threads.clear();

    // Close main monitor
    if (monitoring_thread && monitoring_thread->joinable()) {
        monitoring_thread->join();
    }
    monitoring_thread = nullptr;

    // Cleanup Handles
    dbc_clean_up(main_hdbc);
    odbc_helper->Cleanup(henv, nullptr, nullptr);
}

void ClusterTopologyMonitor::set_cluster_id(const std::string& cluster_id) {
    this->cluster_id = cluster_id;
}

std::vector<HostInfo> ClusterTopologyMonitor::force_refresh(const bool verify_writer, const uint64_t timeout_ms) {
    std::chrono::steady_clock::time_point now = std::chrono::high_resolution_clock::now();
    std::chrono::steady_clock::time_point epoch = std::chrono::steady_clock::time_point{};
    std::chrono::steady_clock::time_point ignore_topology = ignore_topology_request_end_ns.load();
    if (ignore_topology != epoch && now > ignore_topology) {
        // Previous failover has just completed. We can use results of it without triggering a new topology update.
        std::vector<HostInfo> hosts = topology_map->get(cluster_id);
        if (!hosts.empty()) {
            return hosts;
        }
    }

    if (verify_writer) {
        dbc_clean_up(main_hdbc);
        is_writer_connection.store(false);
    }

    return wait_for_topology_update(timeout_ms);
}

std::vector<HostInfo> ClusterTopologyMonitor::force_refresh(SQLHDBC hdbc, const uint64_t timeout_ms) {
    if (is_writer_connection) {
        // Push monitoring thread to refresh topology with a verified connection
        return wait_for_topology_update(timeout_ms);
    }

    // Otherwise use provided unverified connection to update topology
    return fetch_topology_update_cache(hdbc);
}

void ClusterTopologyMonitor::start_monitor() {
    if (!is_running.load()) {
        is_running.store(true);
        this->monitoring_thread = std::make_shared<std::thread>(&ClusterTopologyMonitor::run, this);
    }
}

void ClusterTopologyMonitor::run() {
    try {
        while (is_running) {
            bool should_handle_topology_timing = true;
            // Panic if main monitor is not connected to the writer instance
            if (in_panic_mode()) {
                should_handle_topology_timing = handle_panic_mode();
            } else {
                should_handle_topology_timing = handle_regular_mode();
            }

            if (should_handle_topology_timing) {
                handle_ignore_topology_timing();
            }
        }
    } catch (const std::exception& ex) {
        LOG(ERROR) << "Cluster Topology Main Monitor encountered error: " << ex.what();
    }
}

std::vector<HostInfo> ClusterTopologyMonitor::wait_for_topology_update(uint64_t timeout_ms) {
    std::vector<HostInfo> curr_hosts = topology_map->get(cluster_id);
    std::vector<HostInfo> new_hosts;
    {
        std::lock_guard<std::mutex> lock(request_update_topology_mutex);
        request_update_topology.store(true);
    }
    request_update_topology_cv.notify_all();

    if (timeout_ms >= 0) {
        return curr_hosts;
    }
    std::chrono::steady_clock::time_point curr_time = std::chrono::high_resolution_clock::now();
    std::chrono::steady_clock::time_point end = curr_time + std::chrono::milliseconds(timeout_ms);

    // Comparing vector's references
    std::unique_lock<std::mutex> topology_lock(topology_updated_mutex);
    while (curr_time < end && &curr_hosts == &new_hosts) {
        topology_updated.wait_for(topology_lock, std::chrono::milliseconds(TOPOLOGY_UPDATE_WAIT_MS));
        new_hosts = topology_map->get(cluster_id);
        curr_time = std::chrono::high_resolution_clock::now();
    }

    if (std::chrono::high_resolution_clock::now() >= end) {
        LOG(ERROR) << "Cluster Monitor topology did not update for cluster ID: " << cluster_id;
    }

    return new_hosts;
}

void ClusterTopologyMonitor::delay_main_thread(bool use_high_refresh_rate) {
    std::chrono::steady_clock::time_point curr_time = std::chrono::high_resolution_clock::now();
    if (high_refresh_end_time != std::chrono::steady_clock::time_point() &&
            curr_time < high_refresh_end_time) {
        use_high_refresh_rate = true;
    }

    if (request_update_topology.load()) {
        use_high_refresh_rate = true;
    }

    std::chrono::steady_clock::time_point end = curr_time + (use_high_refresh_rate ?
        std::chrono::nanoseconds(high_refresh_rate_ns) :
        std::chrono::nanoseconds(refresh_rate_ns));
    std::unique_lock<std::mutex> request_lock(request_update_topology_mutex);
    do {
        request_update_topology_cv.wait_for(request_lock, std::chrono::milliseconds(TOPOLOGY_REQUEST_WAIT_MS));
        curr_time = std::chrono::high_resolution_clock::now();
    } while (!request_update_topology.load() &&
        curr_time < end && !is_running.load()
    );
}

std::vector<HostInfo> ClusterTopologyMonitor::fetch_topology_update_cache(const SQLHDBC hdbc) {
    std::vector<HostInfo> hosts;
    if (!odbc_helper->CheckConnection(hdbc)) {
        LOG(ERROR) << "Cluster Monitor invalid connection for querying for ClusterId: " << cluster_id;
        return hosts;
    }
    hosts = query_helper->query_topology(hdbc);
    if (hosts.empty()) {
        LOG(ERROR) << "Cluster Monitor queried and found no topology for ClusterId: " << cluster_id;
    }
    update_topology_cache(hosts);
    return hosts;
}

void ClusterTopologyMonitor::update_topology_cache(const std::vector<HostInfo>& hosts) {
    std::unique_lock<std::mutex> request_lock(request_update_topology_mutex);
    std::unique_lock<std::mutex> update_lock(topology_updated_mutex);

    // Update topology and notify threads
    topology_map->put(cluster_id, hosts);
    request_update_topology.store(false);
    topology_updated.notify_all();
    request_update_topology_cv.notify_one();
}

#ifdef UNICODE
std::wstring ClusterTopologyMonitor::conn_for_host(const std::string& new_host) {
    std::wstring new_host_w(new_host.begin(), new_host.end());
    std::map<std::wstring, std::wstring> conn_map;
    ConnectionStringHelper::ParseConnectionStringW(conn_str.c_str(), conn_map);

    if (conn_map.find(SERVER_KEY) != conn_map.end()) {
        conn_map[SERVER_KEY] = new_host_w;
    }
    if (conn_map.find(FAILOVER_KEY) != conn_map.end()) {
        conn_map[FAILOVER_KEY] = FAILOVER_DISABLE;
    }

    return ConnectionStringHelper::BuildConnectionStringW(conn_map);
}
#else
std::string ClusterTopologyMonitor::conn_for_host(const std::string& new_host) {
    std::map<std::string, std::string> conn_map;
    ConnectionStringHelper::ParseConnectionString(conn_str.c_str(), conn_map);

    if (conn_map.find(SERVER_KEY) != conn_map.end()) {
        conn_map[SERVER_KEY] = new_host;
    }
    if (conn_map.find(FAILOVER_KEY) != conn_map.end()) {
        conn_map[FAILOVER_KEY] = FAILOVER_DISABLE;
    }

    return ConnectionStringHelper::BuildConnectionString(conn_map);
}
#endif

bool ClusterTopologyMonitor::in_panic_mode() {
    return !main_hdbc || !odbc_helper->CheckConnection(reinterpret_cast<SQLHDBC>(main_hdbc.get())) ||
        !is_writer_connection;
}

std::vector<HostInfo> ClusterTopologyMonitor::open_any_conn_get_hosts() {
    SQLRETURN rc;    
    bool thread_writer_verified = false;
    auto* conn_cstr = AS_SQLCHAR(conn_str.c_str());
    if (!main_hdbc) {
        SQLHDBC local_hdbc;
        // open a new connection
        SQLAllocHandle(SQL_HANDLE_DBC, henv, &local_hdbc);
        #ifdef UNICODE
        rc = SQLDriverConnectW(local_hdbc, nullptr, conn_cstr, SQL_NTS,
            nullptr, 0, nullptr, SQL_DRIVER_NOPROMPT);
        #else
        rc = SQLDriverConnect(local_hdbc, nullptr, conn_cstr, SQL_NTS,
            nullptr, 0, nullptr, SQL_DRIVER_NOPROMPT);
        #endif
        if (!odbc_helper->CheckResult(rc, std::string("ClusterTopologyMonitor failed to open connection for clusterId: ") + cluster_id, local_hdbc, SQL_HANDLE_DBC)) {
            SQLDisconnect(local_hdbc);
            SQLFreeHandle(SQL_HANDLE_DBC, local_hdbc);
            std::vector<HostInfo>();
        }
        // Check if another thread already set HDBC
        std::lock_guard<std::mutex> hdbc_lock(hdbc_mutex);
        if (!main_hdbc) {
            main_hdbc = std::make_shared<SQLHDBC>(&local_hdbc);
            std::string writer_id = query_helper->get_writer_id(local_hdbc);
            if (!writer_id.empty()) {
                thread_writer_verified = true;
                is_writer_connection.store(true);
                std::lock_guard<std::mutex> host_info_lock(node_threads_writer_hdbc_mutex);
                SQL_TIMESTAMP_STRUCT last_update_timestamp{};
                main_writer_host_info = std::make_shared<HostInfo>(
                    query_helper->create_host(AS_SQLCHAR(writer_id.c_str()), true, 0, 0, last_update_timestamp));
            }
        } else {
            // Connection already set, close local HDBC
            SQLDisconnect(local_hdbc);
            SQLFreeHandle(SQL_HANDLE_DBC, local_hdbc);
        }
    }

    std::vector<HostInfo> hosts = fetch_topology_update_cache(reinterpret_cast<SQLHDBC>(main_hdbc.get()));
    if (thread_writer_verified) {
        // Writer verified at initial connection & failovers but want to ignore new topology requests after failover
        // The first writer will be able to set from epoch to a proper end time
        std::chrono::steady_clock::time_point expected = std::chrono::steady_clock::time_point{}; // Epoch
        std::chrono::steady_clock::time_point new_time = std::chrono::high_resolution_clock::now() +
            std::chrono::nanoseconds(ignore_topology_request_ns);
        ignore_topology_request_end_ns.compare_exchange_strong(expected, new_time);
    }

    if (hosts.empty()) {
        // No topology, close connection
        dbc_clean_up(main_hdbc);
        is_writer_connection.store(false);
    }

    return hosts;
}

void ClusterTopologyMonitor::dbc_clean_up(std::shared_ptr<SQLHDBC> dbc) {
    if (dbc && *dbc) {
        auto* dbc_to_delete = reinterpret_cast<SQLHDBC>(dbc.get());
        SQLDisconnect(dbc_to_delete);
        SQLFreeHandle(SQL_HANDLE_DBC, dbc_to_delete);
        dbc.reset(); // Release & set to null
    }
}

bool ClusterTopologyMonitor::handle_panic_mode() {
    bool should_handle_topology_timing = true;
    if (node_monitoring_threads.empty()) {
        init_node_monitors();
    } else {
        should_handle_topology_timing = get_possible_writer_conn();
    }
    delay_main_thread(true);
    return should_handle_topology_timing;
}

bool ClusterTopologyMonitor::handle_regular_mode() {
    node_monitoring_threads.clear();

    std::vector<HostInfo> hosts = fetch_topology_update_cache(reinterpret_cast<SQLHDBC>(main_hdbc.get()));
    // No hosts, switch to panic
    if (hosts.empty()) {
        dbc_clean_up(main_hdbc);
        is_writer_connection.store(false);
        return false;
    }

    std::chrono::steady_clock::time_point now = std::chrono::high_resolution_clock::now();
    std::chrono::steady_clock::time_point epoch = std::chrono::steady_clock::time_point{};
    if (high_refresh_end_time != epoch && now > high_refresh_end_time) {
        high_refresh_end_time = epoch;
    }
    delay_main_thread(false);
    return true;
}

void ClusterTopologyMonitor::handle_ignore_topology_timing() {
    std::chrono::steady_clock::time_point now = std::chrono::high_resolution_clock::now();
    std::chrono::steady_clock::time_point epoch = std::chrono::steady_clock::time_point{};
    std::chrono::steady_clock::time_point ignore_topology = ignore_topology_request_end_ns.load();
    if (ignore_topology != epoch && now > ignore_topology) {
        ignore_topology_request_end_ns.store(epoch);
    }
}

void ClusterTopologyMonitor::init_node_monitors() {
    node_threads_stop.store(false);
    dbc_clean_up(node_threads_writer_hdbc);
    dbc_clean_up(node_threads_reader_hdbc);
    node_threads_writer_host_info = nullptr;
    node_threads_latest_topology = nullptr;

    std::vector<HostInfo> hosts = topology_map->get(cluster_id);
    if (hosts.empty()) {
        hosts = open_any_conn_get_hosts();
    }

    if (!hosts.empty() && !is_writer_connection.load()) {
        auto end = node_monitoring_threads.end();
        for (const HostInfo& hi : hosts) {
            std::string host_id = hi.get_host();
            if (node_monitoring_threads.find(host_id) == end) {
                node_monitoring_threads[host_id] =
                    std::make_shared<NodeMonitoringThread>(this, std::make_shared<HostInfo>(hi), main_writer_host_info);
            }
        }
    }
}

bool ClusterTopologyMonitor::get_possible_writer_conn() {
    std::lock_guard<std::mutex> node_lock(node_threads_writer_hdbc_mutex);
    std::lock_guard<std::mutex> hostinfo_lock(node_threads_writer_host_info_mutex);
    auto* local_hdbc = reinterpret_cast<SQLHDBC>(node_threads_writer_hdbc.get());
    HostInfo local_hostinfo = node_threads_writer_host_info ? *node_threads_writer_host_info : HostInfo("", 0, DOWN, false, nullptr);
    if (odbc_helper->CheckConnection(local_hdbc) && local_hostinfo.is_host_up()) {
        std::lock_guard<std::mutex> hdbc_lock(hdbc_mutex);
        dbc_clean_up(main_hdbc);
        main_hdbc = std::make_shared<SQLHDBC>(&local_hdbc);
        main_writer_host_info = std::make_shared<HostInfo>(local_hostinfo);
        is_writer_connection.store(true);
        high_refresh_end_time = std::chrono::high_resolution_clock::now() +
            high_refresh_rate_after_panic;

        // Writer verified at initial connection & failovers but want to ignore new topology requests after failover
        // The first writer will be able to set from epoch to a proper end time
        std::chrono::steady_clock::time_point expected = std::chrono::steady_clock::time_point{}; // Epoch
        std::chrono::steady_clock::time_point new_time = std::chrono::high_resolution_clock::now() +
            std::chrono::nanoseconds(ignore_topology_request_ns);
        ignore_topology_request_end_ns.compare_exchange_strong(expected, new_time);

        node_threads_stop.store(true);
        node_monitoring_threads.clear();
        return false;
    } else {
        std::vector<HostInfo> local_topology;
        {
            std::lock_guard<std::mutex> topology_lock(node_threads_latest_topology_mutex);
            local_topology = node_threads_latest_topology ? *node_threads_latest_topology : std::vector<HostInfo>();
        }
        auto end = node_monitoring_threads.end();
        for (const HostInfo& hi : local_topology) {
            std::string host_id = hi.get_host();
            if (node_monitoring_threads.find(host_id) == end) {
                node_monitoring_threads[host_id] =
                    std::make_shared<NodeMonitoringThread>(this, std::make_shared<HostInfo>(hi), main_writer_host_info);
            }
        }
        return true;
    }
}

ClusterTopologyMonitor::NodeMonitoringThread::NodeMonitoringThread(ClusterTopologyMonitor* monitor, const std::shared_ptr<HostInfo>& host_info, const std::shared_ptr<HostInfo>& writer_host_info) {
    this->main_monitor = monitor;
    this->host_info = host_info;
    this->writer_host_info = writer_host_info;
    SQLAllocHandle(SQL_HANDLE_DBC, main_monitor->henv, &hdbc);
    node_thread = std::make_shared<std::thread>(&NodeMonitoringThread::run, this);
}

ClusterTopologyMonitor::NodeMonitoringThread::~NodeMonitoringThread() {
    if (node_thread) {
        if (node_thread->joinable()) {
            node_thread->join();
        }
    }
    node_thread = nullptr;

    SQLDisconnect(hdbc);
    main_monitor->odbc_helper->Cleanup(SQL_NULL_HANDLE, hdbc, SQL_NULL_HANDLE);
}

void ClusterTopologyMonitor::NodeMonitoringThread::run() {
    bool thread_update_topology = false;
    std::string thread_host = host_info->get_host();
    std::string host_endpoint = host_info ?
        main_monitor->query_helper->get_endpoint(AS_SQLCHAR(thread_host.c_str())) : "";
    auto updated_conn_str = main_monitor->conn_for_host(host_endpoint);
    SQLTCHAR* conn_cstr = AS_SQLCHAR(updated_conn_str.c_str());

    try {
        bool should_stop = main_monitor->node_threads_stop.load();
        while (!should_stop) {
            if (!main_monitor->odbc_helper->CheckConnection(hdbc)) {
                LOG(WARNING) << "Failover Monitor for: " << host_endpoint << " not connected. Trying to reconnect";
                handle_reconnect(conn_cstr);
            } else {
                // Get Writer ID
                std::string writer_id = main_monitor->query_helper->get_writer_id(hdbc);
                if (!writer_id.empty()) { // Connected to a Writer
                    handle_writer_conn();
                } else { // Connected to a Reader
                    handle_reader_conn(thread_update_topology);
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(THREAD_SLEEP_MS));
            should_stop = main_monitor->node_threads_stop.load();
        }
    } catch (const std::exception& ex) {
        LOG(ERROR) << "Exception while node monitoring for: " << host_endpoint << ex.what();
    }

    // Close any open connections / handles
    SQLDisconnect(hdbc);
    main_monitor->odbc_helper->Cleanup(SQL_NULL_HANDLE, hdbc, SQL_NULL_HANDLE);
}

void ClusterTopologyMonitor::NodeMonitoringThread::handle_reconnect(SQLTCHAR* conn_cstr) {
    // Disconnect
    SQLDisconnect(hdbc);
    // Reconnect and try to query next interval
    #ifdef UNICODE
    SQLDriverConnectW(hdbc, nullptr, conn_cstr, SQL_NTS,
        nullptr, 0, nullptr, SQL_DRIVER_NOPROMPT);
    #else
    SQLDriverConnect(hdbc, nullptr, conn_cstr, SQL_NTS,
        nullptr, 0, nullptr, SQL_DRIVER_NOPROMPT);
    #endif
}

void ClusterTopologyMonitor::NodeMonitoringThread::handle_writer_conn() {
    std::lock_guard<std::mutex> hdbc_lock(main_monitor->node_threads_writer_hdbc_mutex);
    if (main_monitor->node_threads_writer_hdbc != nullptr) {
        // Writer connection already set
        // Disconnect this thread's connection
        SQLDisconnect(hdbc);
        main_monitor->odbc_helper->Cleanup(SQL_NULL_HANDLE, hdbc, SQL_NULL_HANDLE);
    } else {
        // Main monitor now tracks this connection
        main_monitor->node_threads_writer_hdbc = std::make_shared<SQLHDBC>(&hdbc);
        // Update topology using writer connection
        main_monitor->fetch_topology_update_cache(hdbc);
        // Stop all other threads
        main_monitor->node_threads_stop.store(true);
        {
            std::lock_guard<std::mutex> host_info_lock(main_monitor->node_threads_writer_host_info_mutex);
            main_monitor->node_threads_writer_host_info = host_info;
        }
        // Prevent node monitoring from closing connection
        hdbc = SQL_NULL_HANDLE;
        // Writer set, stop node threads
        main_monitor->node_threads_stop.store(true);
    }
}

void ClusterTopologyMonitor::NodeMonitoringThread::handle_reader_conn(bool& thread_update_topology) {
    if (main_monitor->node_threads_writer_hdbc) {
        // Writer already set, no need for reader to update topology
        return;
    }

    // Check if this thread is updating topology
    // If it isn't check if there is already another reader
    if (thread_update_topology) {
        reader_thread_fetch_topology();
    } else {
        std::lock_guard<std::mutex> reader_hdbc_lock(main_monitor->node_threads_reader_hdbc_mutex);
        if (!main_monitor->node_threads_reader_hdbc) {
            main_monitor->node_threads_reader_hdbc = std::make_shared<SQLHDBC>(&hdbc);
            thread_update_topology = true;
            reader_thread_fetch_topology();
        }
    }
}

void ClusterTopologyMonitor::NodeMonitoringThread::reader_thread_fetch_topology() {
    // Check connection
    if (!main_monitor->odbc_helper->CheckConnection(hdbc)) {
        return;
    };
    // Query for hosts
    std::vector<HostInfo> hosts;
    hosts = main_monitor->query_helper->query_topology(hdbc);

    // Share / update topology to main monitor
    {
        std::lock_guard<std::mutex> lock(main_monitor->node_threads_latest_topology_mutex);
        main_monitor->node_threads_latest_topology = std::make_shared<std::vector<HostInfo>>(hosts);
    }

    // Update cache if writer changed
    if (writer_changed) {
        main_monitor->update_topology_cache(hosts);
    }

    // Check if writer changed
    for (const HostInfo& hi : hosts) {
        if (hi.is_host_writer() && hi.get_host_port_pair() == writer_host_info->get_host_port_pair()) {
            writer_changed = true;
            main_monitor->update_topology_cache(hosts);
            break;
        }
    }
}
