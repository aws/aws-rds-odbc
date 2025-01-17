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
        const std::shared_ptr<SlidingCacheMap<std::string, std::vector<HostInfo>>> topology_map,
        const std::string& conn_str,
        std::shared_ptr<ClusterTopologyQueryHelper> query_helper,
        const long ignore_topology_request_ns,
        const long high_refresh_rate_ns,
        const long refresh_rate_ns):
        cluster_id{ std::move(cluster_id) },
        topology_map { std::move(topology_map) },
        conn_str { std::move(conn_str) },
        query_helper { query_helper },
        ignore_topology_request_ns { ignore_topology_request_ns },
        high_refresh_rate_ns { high_refresh_rate_ns },
        refresh_rate_ns { refresh_rate_ns } {

    if (!OdbcHelper::CheckResult(SQLAllocHandle(SQL_HANDLE_ENV, nullptr, &henv),
        std::string("Cluster Topology Monitor unable to allocate environment handle for: ") + cluster_id,
        henv, SQL_HANDLE_ENV)) {
        return;
    }
    is_running = true;
    this->monitoring_thread = std::make_shared<std::thread>(&ClusterTopologyMonitor::run, this);
}

ClusterTopologyMonitor::~ClusterTopologyMonitor() {
    is_running.store(false);

    // Notify node threads if they are stuck waiting
    {
        std::lock_guard<std::mutex> lock(request_update_topology_mutex);
        request_update_topology.store(true);
    }
    request_update_topology_cv.notify_all();

    // Close main monitor
    if (monitoring_thread && monitoring_thread->joinable()) {
        monitoring_thread->join();
    }
    monitoring_thread = nullptr;

    // Cleanup Handles
    if (main_hdbc) {
        SQLHDBC dbc_to_delete = main_hdbc.get();
        SQLDisconnect(dbc_to_delete);
        SQLFreeHandle(SQL_HANDLE_DBC, dbc_to_delete);
        main_hdbc = nullptr;
    }
    OdbcHelper::Cleanup(henv, nullptr, nullptr);
}

void ClusterTopologyMonitor::set_cluster_id(std::string cluster_id) {
    this->cluster_id = cluster_id;
}

std::vector<HostInfo> ClusterTopologyMonitor::force_refresh(const bool verify_writer, const long timeout_ms) {
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
        if (main_hdbc) {
            SQLHDBC dbc_to_delete = main_hdbc.get();
            SQLDisconnect(dbc_to_delete);
            SQLFreeHandle(SQL_HANDLE_DBC, dbc_to_delete);
            main_hdbc = nullptr;
        }
        is_writer_connection.store(false);
    }

    return wait_for_topology_update(timeout_ms);
}

std::vector<HostInfo> ClusterTopologyMonitor::force_refresh(SQLHDBC hdbc, const long timeout_ms) {
    if (is_writer_connection) {
        // Push monitoring thread to refresh topology with a verified connection
        return wait_for_topology_update(timeout_ms);
    }

    // Otherwise use provided unverified connection to update topology
    return fetch_topology_update_cache(hdbc);
}

void ClusterTopologyMonitor::run() {
    try {
        while (is_running) {
            if (in_panic_mode()) {
                // Spin up node threads
                if (node_monitoring_threads.empty()) {
                    node_threads_stop.store(false);
                    if (node_threads_writer_hdbc) {
                        SQLHDBC dbc_to_delete = *node_threads_reader_hdbc.get();
                        SQLDisconnect(dbc_to_delete);
                        SQLFreeHandle(SQL_HANDLE_DBC, dbc_to_delete);
                        node_threads_writer_hdbc = nullptr;
                    }
                    if (node_threads_reader_hdbc) {
                        SQLHDBC dbc_to_delete = *node_threads_reader_hdbc.get();
                        SQLDisconnect(dbc_to_delete);
                        SQLFreeHandle(SQL_HANDLE_DBC, dbc_to_delete);
                        node_threads_reader_hdbc = nullptr;                        
                    }
                    node_threads_writer_host_info = nullptr;
                    node_threads_latest_topology = nullptr;

                    std::vector<HostInfo> hosts = topology_map->get(cluster_id);
                    if (hosts.empty()) {
                        hosts = open_any_conn_get_hosts();
                    }

                    if (!hosts.empty() && !is_writer_connection.load()) {
                        for (HostInfo hi : hosts) {
                            node_monitoring_threads[hi.get_host()] =
                                std::make_shared<NodeMonitoringThread>(this, std::make_shared<HostInfo>(hi), main_writer_host_info, conn_str);
                        }
                    }
                } else {
                    std::lock_guard<std::mutex> node_lock(node_threads_writer_hdbc_mutex);
                    std::lock_guard<std::mutex> hostinfo_lock(node_threads_writer_host_info_mutex);
                    SQLHDBC local_hdbc = *node_threads_writer_hdbc.get();
                    HostInfo local_hostinfo = *node_threads_writer_host_info.get();
                    if (OdbcHelper::CheckConnection(local_hdbc) && local_hostinfo.is_host_up()) {
                        std::lock_guard<std::mutex> hdbc_lock(hdbc_mutex);
                        if (main_hdbc) {
                            SQLHDBC dbc_to_delete = *main_hdbc.get();
                            SQLDisconnect(dbc_to_delete);
                            SQLFreeHandle(SQL_HANDLE_DBC, dbc_to_delete);
                            main_hdbc = nullptr;
                        }
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
                        continue;
                    } else {
                        std::lock_guard<std::mutex> topology_lock(node_threads_latest_topology_mutex);
                        std::vector<HostInfo> local_topology = *node_threads_latest_topology.get();
                        auto end = node_monitoring_threads.end();
                        for (HostInfo hi : local_topology) {
                            std::string host_id = hi.get_host();
                            if (node_monitoring_threads.find(host_id) != end) {
                                node_monitoring_threads[host_id] =
                                    std::make_shared<NodeMonitoringThread>(this, std::make_shared<HostInfo>(hi), main_writer_host_info, conn_str);
                            }
                        }
                    }
                }
                delay(true);
            } else {
                // Regular / Non-Panic Mode
                node_monitoring_threads.clear();

                std::vector<HostInfo> hosts = fetch_topology_update_cache(*main_hdbc.get());
                // No hosts, switch to panic
                if (hosts.empty()) {
                    if (main_hdbc) {
                        SQLHDBC dbc_to_delete = *main_hdbc.get();
                        SQLDisconnect(dbc_to_delete);
                        SQLFreeHandle(SQL_HANDLE_DBC, dbc_to_delete);
                        main_hdbc = nullptr;
                    }
                    is_writer_connection.store(false);
                    continue;
                }

                std::chrono::steady_clock::time_point now = std::chrono::high_resolution_clock::now();
                std::chrono::steady_clock::time_point epoch = std::chrono::steady_clock::time_point{};
                if (high_refresh_end_time != epoch && now > high_refresh_end_time) {
                    high_refresh_end_time = epoch;
                }
                delay(false);
            }

            std::chrono::steady_clock::time_point now = std::chrono::high_resolution_clock::now();
            std::chrono::steady_clock::time_point epoch = std::chrono::steady_clock::time_point{};
            std::chrono::steady_clock::time_point ignore_topology = ignore_topology_request_end_ns.load();
            if (ignore_topology != epoch && now > ignore_topology) {
                ignore_topology_request_end_ns.store(epoch);
            }
        }
    } catch (const std::exception& ex) {
        LOG(ERROR) << "Cluster Topology Main Monitor encountered error: " << ex.what();
    }
}

std::vector<HostInfo> ClusterTopologyMonitor::wait_for_topology_update(long timeout_ms) {
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
    std::chrono::steady_clock::time_point end = std::chrono::high_resolution_clock::now() +
        std::chrono::milliseconds(timeout_ms);

    // Comparing vector's references
    while (std::chrono::high_resolution_clock::now() < end && &curr_hosts == &(new_hosts = topology_map->get(cluster_id))) {
        std::unique_lock<std::mutex> topology_lock(topology_updated_mutex, std::defer_lock);
        while (std::chrono::high_resolution_clock::now() < end &&
            &curr_hosts == &(new_hosts = topology_map->get(cluster_id))) {
            topology_updated.wait_for(topology_lock, std::chrono::milliseconds(1000));
        }
    }

    if (std::chrono::high_resolution_clock::now() >= end) {
        LOG(ERROR) << "Cluster Monitor topology did not update for cluster ID: " << cluster_id;
    }

    return new_hosts;
}

void ClusterTopologyMonitor::delay(bool use_high_refresh_rate) {
    std::chrono::steady_clock::time_point now = std::chrono::high_resolution_clock::now();
    if (high_refresh_end_time != std::chrono::steady_clock::time_point() &&
            now < high_refresh_end_time) {
        use_high_refresh_rate = true;
    }

    if (request_update_topology.load()) {
        use_high_refresh_rate = true;
    }

    std::chrono::steady_clock::time_point end = now + (use_high_refresh_rate ?
        std::chrono::nanoseconds(high_refresh_rate_ns) :
        std::chrono::nanoseconds(refresh_rate_ns));
    std::unique_lock<std::mutex> request_lock(request_update_topology_mutex, std::defer_lock);
    do {
        request_update_topology_cv.wait_for(request_lock, std::chrono::milliseconds(50));
    } while (!request_update_topology.load() &&
        std::chrono::high_resolution_clock::now() < end &&
        !is_running.load());
}

std::vector<HostInfo> ClusterTopologyMonitor::fetch_topology_update_cache(const SQLHDBC hdbc) {
    std::vector<HostInfo> hosts;
    if (!OdbcHelper::CheckConnection(hdbc)) {
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
    std::unique_lock<std::mutex> request_lock(request_update_topology_mutex, std::defer_lock);
    std::unique_lock<std::mutex> update_lock(topology_updated_mutex, std::defer_lock);

    // Try to get lock, if not wait under condition variable
    while (true) {
        if (request_lock.try_lock()) {
            // Update topology and notify threads
            topology_map->put(cluster_id, hosts);
            std::lock_guard<std::mutex> update_lock(topology_updated_mutex);
            request_update_topology.store(false);
            topology_updated.notify_all();
            break;
        } else {
            request_update_topology_cv.wait(request_lock);
        }
    }

    // Release lock & notify next CV if other threads are waiting
    request_lock.unlock();
    request_update_topology_cv.notify_one();
}

std::string ClusterTopologyMonitor::conn_str_replace_host(std::string conn_in, std::string new_host) {
    std::map<std::string, std::string> conn_map;
    ConnectionStringHelper::ParseConnectionString(conn_in.c_str(), conn_map);

    if (conn_map.find("SERVER") != conn_map.end()) {
        conn_map["SERVER"] = new_host;
    }
    // TODO - Disable Failover

    return ConnectionStringHelper::BuildConnectionString(conn_map);
}

bool ClusterTopologyMonitor::in_panic_mode() {
    return !main_hdbc || !OdbcHelper::CheckConnection(*main_hdbc.get()) ||
        !is_writer_connection;
}

std::vector<HostInfo> ClusterTopologyMonitor::open_any_conn_get_hosts() {
    SQLRETURN rc;    
    bool thread_writer_verified = false;
    if (!main_hdbc) {
        SQLHDBC local_hdbc;
        // open a new connection
        SQLAllocHandle(SQL_HANDLE_DBC, henv, &local_hdbc);
        SQLTCHAR* conn_cstr = const_cast<SQLTCHAR*>(reinterpret_cast<const SQLTCHAR*>(conn_str.c_str()));
        rc = SQLDriverConnect(local_hdbc, nullptr, conn_cstr, SQL_NTS,
            nullptr, 0, nullptr, SQL_DRIVER_NOPROMPT);
        if (!OdbcHelper::CheckResult(rc, std::string("ClusterTopologyMonitor failed to open connection for clusterId: ") + cluster_id, local_hdbc, SQL_HANDLE_DBC)) {
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
                SQLTCHAR* writer_id_cstr = new SQLTCHAR[writer_id.size() + 1];
                std::copy(writer_id.begin(), writer_id.end(), writer_id_cstr);
                writer_id_cstr[writer_id.size()] = '\0';
                SQL_TIMESTAMP_STRUCT last_update_timestamp;
                main_writer_host_info = std::make_shared<HostInfo>(
                    query_helper->create_host(writer_id_cstr, true, 0, 0, last_update_timestamp));
            }
        } else {
            // Connection already set, close local HDBC
            SQLDisconnect(local_hdbc);
            SQLFreeHandle(SQL_HANDLE_DBC, local_hdbc);
        }
    }

    std::vector<HostInfo> hosts = fetch_topology_update_cache(*main_hdbc.get());
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
        if (main_hdbc) {
            SQLHDBC dbc_to_delete = *main_hdbc.get();
            SQLDisconnect(dbc_to_delete);
            SQLFreeHandle(SQL_HANDLE_DBC, dbc_to_delete);
            main_hdbc = nullptr;
        }
        is_writer_connection.store(false);
    }

    return hosts;
}

ClusterTopologyMonitor::NodeMonitoringThread::NodeMonitoringThread(ClusterTopologyMonitor* monitor, std::shared_ptr<HostInfo> host_info, std::shared_ptr<HostInfo> writer_host_info, std::string conn_str) {
    main_monitor = monitor;
    this->host_info = host_info;
    this->writer_host_info = host_info;
    this->conn_str = conn_str;
    node_thread = std::make_shared<std::thread>(&NodeMonitoringThread::run, this);
}

ClusterTopologyMonitor::NodeMonitoringThread::~NodeMonitoringThread() {
    if (node_thread) {
        if (node_thread->joinable()) {
            node_thread->join();
        }
    }
    node_thread = nullptr;

    if (hdbc) {
        SQLDisconnect(hdbc);
    }
    OdbcHelper::Cleanup(SQL_NULL_HANDLE, hdbc, SQL_NULL_HANDLE);
}

void ClusterTopologyMonitor::NodeMonitoringThread::run() {
    bool thread_update_topology = false;
    std::string host_endpoint = host_info ? host_info->get_host() : "";
    conn_str = main_monitor->conn_str_replace_host(conn_str, host_endpoint);
    SQLTCHAR* conn_cstr = const_cast<SQLTCHAR*>(reinterpret_cast<const SQLTCHAR*>(conn_str.c_str()));
    try {
        while (!main_monitor->node_threads_stop.load()) {
            if (!OdbcHelper::CheckConnection(hdbc)) {
                // Disconnect
                LOG(WARNING) << "Failover Monitor for: " << host_endpoint << " not connected. Trying to reconnect";
                SQLDisconnect(hdbc);
                OdbcHelper::Cleanup(SQL_NULL_HANDLE, hdbc, SQL_NULL_HANDLE);
                // Reconnect and try to query next interval
                SQLAllocHandle(SQL_HANDLE_DBC, main_monitor->henv, &hdbc);
                SQLDriverConnect(hdbc, nullptr, conn_cstr, SQL_NTS,
                    nullptr, 0, nullptr, SQL_DRIVER_NOPROMPT);
            } else {
                // Get Writer ID
                std::string writer_id = main_monitor->query_helper->get_writer_id(hdbc);
                if (!writer_id.empty()) { // Connected to a Writer
                    std::lock_guard<std::mutex> hdbc_lock(main_monitor->node_threads_writer_hdbc_mutex);
                    if (main_monitor->node_threads_writer_hdbc != nullptr) {
                        // Writer connection already set
                        // Disconnect this thread's connection
                        SQLDisconnect(hdbc);
                        OdbcHelper::Cleanup(SQL_NULL_HANDLE, hdbc, SQL_NULL_HANDLE);
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
                    }
                    return;
                } else { // Connected to a Reader
                    // Update topology if writer connection is not set
                    if (!main_monitor->node_threads_writer_hdbc) {
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
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(THREAD_SLEEP_MS));
        }
    } catch (const std::exception& ex) {
        LOG(ERROR) << "Exception while node monitoring for: " << host_endpoint << ex.what();
    }

    // Close any open connections / handles
    if (hdbc) {
        SQLDisconnect(hdbc);
    }
    OdbcHelper::Cleanup(SQL_NULL_HANDLE, hdbc, SQL_NULL_HANDLE);
}

void ClusterTopologyMonitor::NodeMonitoringThread::reader_thread_fetch_topology() {
    // Check connection
    if (!OdbcHelper::CheckConnection(hdbc)) {
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
    HostInfo* latest_writer = nullptr;
    for (HostInfo& hi : hosts) {
        if (hi.is_host_writer()) {
            latest_writer = &hi;
            break;
        }
    }
    if (latest_writer && writer_host_info &&
            latest_writer->get_host_port_pair() == writer_host_info->get_host_port_pair()) {
        writer_changed = true;
        main_monitor->update_topology_cache(hosts);
    }
}
