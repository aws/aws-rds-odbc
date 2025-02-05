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
        cluster_id_{ cluster_id },
        topology_map_ { topology_map },
        odbc_helper_ { std::move(odbc_helper) },
        query_helper_ { std::move(query_helper) },
        ignore_topology_request_ns_ { ignore_topology_request_ns },
        high_refresh_rate_ns_ { high_refresh_rate_ns },
        refresh_rate_ns_ { refresh_rate_ns } {

    if (!this->odbc_helper_->CheckResult(SQLAllocHandle(SQL_HANDLE_ENV, nullptr, &henv_),
        std::string("Cluster Topology Monitor unable to allocate environment handle for: ") + cluster_id,
            henv_, SQL_HANDLE_ENV)) {
        throw std::runtime_error(std::string("Cluster Topology Monitor unable to allocate HENV for ClusterId: ") + cluster_id);
    }
    #ifdef UNICODE
    conn_str_ = std::wstring(reinterpret_cast<const wchar_t*>(conn_cstr));
    #else
    conn_str_ = std::string(reinterpret_cast<const char*>(conn_cstr));
    #endif
}

ClusterTopologyMonitor::~ClusterTopologyMonitor() {
    is_running_.store(false);
    node_threads_stop_.store(true);

    // Notify node threads if they are stuck waiting
    {
        std::lock_guard<std::mutex> lock(request_update_topology_mutex_);
        request_update_topology_.store(true);
    }
    request_update_topology_cv_.notify_all();
    node_monitoring_threads_.clear();

    // Close main monitor
    if (monitoring_thread_ && monitoring_thread_->joinable()) {
        monitoring_thread_->join();
    }
    monitoring_thread_ = nullptr;

    // Cleanup Handles
    dbc_clean_up(main_hdbc_);
    odbc_helper_->Cleanup(henv_, nullptr, nullptr);
}

void ClusterTopologyMonitor::set_cluster_id(const std::string& cluster_id) {
    this->cluster_id_ = cluster_id;
}

std::vector<HostInfo> ClusterTopologyMonitor::force_refresh(const bool verify_writer, const uint64_t timeout_ms) {
    std::chrono::steady_clock::time_point now = std::chrono::high_resolution_clock::now();
    std::chrono::steady_clock::time_point epoch = std::chrono::steady_clock::time_point{};
    std::chrono::steady_clock::time_point ignore_topology = ignore_topology_request_end_ns_.load();
    if (ignore_topology != epoch && now > ignore_topology) {
        // Previous failover has just completed. We can use results of it without triggering a new topology update.
        std::vector<HostInfo> hosts = topology_map_->get(cluster_id_);
        if (!hosts.empty()) {
            return hosts;
        }
    }

    if (verify_writer) {
        dbc_clean_up(main_hdbc_);
        is_writer_connection_.store(false);
    }

    return wait_for_topology_update(timeout_ms);
}

std::vector<HostInfo> ClusterTopologyMonitor::force_refresh(SQLHDBC hdbc, const uint64_t timeout_ms) {
    if (is_writer_connection_) {
        // Push monitoring thread to refresh topology with a verified connection
        return wait_for_topology_update(timeout_ms);
    }

    // Otherwise use provided unverified connection to update topology
    return fetch_topology_update_cache(hdbc);
}

void ClusterTopologyMonitor::start_monitor() {
    if (!is_running_.load()) {
        is_running_.store(true);
        this->monitoring_thread_ = std::make_shared<std::thread>(&ClusterTopologyMonitor::run, this);
    }
}

void ClusterTopologyMonitor::run() {
    try {
        while (is_running_) {
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
    std::vector<HostInfo> curr_hosts = topology_map_->get(cluster_id_);
    std::vector<HostInfo> new_hosts;
    {
        std::lock_guard<std::mutex> lock(request_update_topology_mutex_);
        request_update_topology_.store(true);
    }
    request_update_topology_cv_.notify_all();

    if (timeout_ms == 0) {
        LOG(INFO) << "Cluster Monitor topology skipping wait period for topology update";
        return curr_hosts;
    }
    std::chrono::steady_clock::time_point curr_time = std::chrono::high_resolution_clock::now();
    std::chrono::steady_clock::time_point end = curr_time + std::chrono::milliseconds(timeout_ms);

    // Comparing vector's references
    std::unique_lock<std::mutex> topology_lock(topology_updated_mutex_);
    while (curr_time < end && &curr_hosts == &new_hosts) {
        topology_updated_.wait_for(topology_lock, std::chrono::milliseconds(TOPOLOGY_UPDATE_WAIT_MS));
        new_hosts = topology_map_->get(cluster_id_);
        curr_time = std::chrono::high_resolution_clock::now();
    }

    if (std::chrono::high_resolution_clock::now() >= end) {
        LOG(ERROR) << "Cluster Monitor topology did not update for cluster ID: " << cluster_id_;
    }

    return new_hosts;
}

void ClusterTopologyMonitor::delay_main_thread(bool use_high_refresh_rate) {
    std::chrono::steady_clock::time_point curr_time = std::chrono::high_resolution_clock::now();
    if ((high_refresh_end_time_ != std::chrono::steady_clock::time_point() &&
            curr_time < high_refresh_end_time_) || request_update_topology_.load()) {
        use_high_refresh_rate = true;
    }

    std::chrono::steady_clock::time_point end = curr_time + (use_high_refresh_rate ?
        std::chrono::nanoseconds(high_refresh_rate_ns_) :
        std::chrono::nanoseconds(refresh_rate_ns_));
    std::unique_lock<std::mutex> request_lock(request_update_topology_mutex_);
    do {
        request_update_topology_cv_.wait_for(request_lock, std::chrono::milliseconds(TOPOLOGY_REQUEST_WAIT_MS));
        curr_time = std::chrono::high_resolution_clock::now();
    } while (!request_update_topology_.load() &&
        curr_time < end && !is_running_.load()
    );
}

std::vector<HostInfo> ClusterTopologyMonitor::fetch_topology_update_cache(const SQLHDBC hdbc) {
    std::vector<HostInfo> hosts;
    if (!odbc_helper_->CheckConnection(hdbc)) {
        LOG(ERROR) << "Cluster Monitor invalid connection for querying for ClusterId: " << cluster_id_;
        return hosts;
    }
    hosts = query_helper_->query_topology(hdbc);
    if (hosts.empty()) {
        LOG(ERROR) << "Cluster Monitor queried and found no topology for ClusterId: " << cluster_id_;
    } else {
        // Update if new topology is found
        update_topology_cache(hosts);
    }
    
    return hosts;
}

void ClusterTopologyMonitor::update_topology_cache(const std::vector<HostInfo>& hosts) {
    std::unique_lock<std::mutex> request_lock(request_update_topology_mutex_);
    std::unique_lock<std::mutex> update_lock(topology_updated_mutex_);

    // Update topology and notify threads
    topology_map_->put(cluster_id_, hosts);
    request_update_topology_.store(false);
    topology_updated_.notify_all();
    request_update_topology_cv_.notify_one();
}

#ifdef UNICODE
std::wstring ClusterTopologyMonitor::conn_for_host(const std::string& new_host) {
    std::wstring new_host_w(new_host.begin(), new_host.end());
    std::map<std::wstring, std::wstring> conn_map;
    ConnectionStringHelper::ParseConnectionStringW(conn_str_.c_str(), conn_map);

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
    ConnectionStringHelper::ParseConnectionString(conn_str_.c_str(), conn_map);

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
    return !main_hdbc_ || !odbc_helper_->CheckConnection(reinterpret_cast<SQLHDBC>(*(main_hdbc_.get()))) ||
        !is_writer_connection_;
}

std::vector<HostInfo> ClusterTopologyMonitor::open_any_conn_get_hosts() {
    SQLRETURN rc;    
    bool thread_writer_verified = false;
    auto* conn_cstr = AS_SQLTCHAR(conn_str_.c_str());
    if (!main_hdbc_) {
        SQLHDBC local_hdbc;
        // open a new connection
        SQLAllocHandle(SQL_HANDLE_DBC, henv_, &local_hdbc);
        #ifdef UNICODE
        rc = SQLDriverConnectW(local_hdbc, nullptr, conn_cstr, SQL_NTS,
            nullptr, 0, nullptr, SQL_DRIVER_NOPROMPT);
        #else
        rc = SQLDriverConnect(local_hdbc, nullptr, conn_cstr, SQL_NTS,
            nullptr, 0, nullptr, SQL_DRIVER_NOPROMPT);
        #endif
        if (!odbc_helper_->CheckResult(rc, std::string("ClusterTopologyMonitor failed to open connection for clusterId: ") + cluster_id_, local_hdbc, SQL_HANDLE_DBC)) {
            SQLDisconnect(local_hdbc);
            SQLFreeHandle(SQL_HANDLE_DBC, local_hdbc);
            return std::vector<HostInfo>();
        }
        // Check if another thread already set HDBC
        std::lock_guard<std::mutex> hdbc_lock(hdbc_mutex_);
        if (!main_hdbc_) {
            main_hdbc_ = std::make_shared<SQLHDBC>(local_hdbc);
            std::string writer_id = query_helper_->get_writer_id(local_hdbc);
            if (!writer_id.empty()) {
                thread_writer_verified = true;
                is_writer_connection_.store(true);
                std::lock_guard<std::mutex> host_info_lock(node_threads_writer_hdbc_mutex_);
                SQL_TIMESTAMP_STRUCT last_update_timestamp{};
                main_writer_host_info_ = std::make_shared<HostInfo>(
                    query_helper_->create_host(AS_SQLTCHAR(writer_id.c_str()), true, 0, 0, last_update_timestamp));
            }
        } else {
            // Connection already set, close local HDBC
            SQLDisconnect(local_hdbc);
            SQLFreeHandle(SQL_HANDLE_DBC, local_hdbc);
        }
    }

    std::vector<HostInfo> hosts = fetch_topology_update_cache(reinterpret_cast<SQLHDBC>(*(main_hdbc_.get())));
    if (thread_writer_verified) {
        // Writer verified at initial connection & failovers but want to ignore new topology requests after failover
        // The first writer will be able to set from epoch to a proper end time
        std::chrono::steady_clock::time_point expected = std::chrono::steady_clock::time_point{}; // Epoch
        std::chrono::steady_clock::time_point new_time = std::chrono::high_resolution_clock::now() +
            std::chrono::nanoseconds(ignore_topology_request_ns_);
        ignore_topology_request_end_ns_.compare_exchange_strong(expected, new_time);
    }

    if (hosts.empty()) {
        // No topology, close connection
        dbc_clean_up(main_hdbc_);
        is_writer_connection_.store(false);
    }

    return hosts;
}

void ClusterTopologyMonitor::dbc_clean_up(std::shared_ptr<SQLHDBC>& dbc) {
    if (dbc && dbc.get()) {
        auto* dbc_to_delete = reinterpret_cast<SQLHDBC>(*(dbc.get()));
        SQLDisconnect(dbc_to_delete);
        SQLFreeHandle(SQL_HANDLE_DBC, dbc_to_delete);
        dbc.reset(); // Release & set to null
    }
}

bool ClusterTopologyMonitor::handle_panic_mode() {
    bool should_handle_topology_timing = true;
    if (node_monitoring_threads_.empty()) {
        init_node_monitors();
    } else {
        should_handle_topology_timing = get_possible_writer_conn();
    }
    delay_main_thread(true);
    return should_handle_topology_timing;
}

bool ClusterTopologyMonitor::handle_regular_mode() {
    node_monitoring_threads_.clear();

    std::vector<HostInfo> hosts = fetch_topology_update_cache(reinterpret_cast<SQLHDBC>(*main_hdbc_.get()));
    // No hosts, switch to panic
    if (hosts.empty()) {
        dbc_clean_up(main_hdbc_);
        is_writer_connection_.store(false);
        return false;
    }

    std::chrono::steady_clock::time_point now = std::chrono::high_resolution_clock::now();
    std::chrono::steady_clock::time_point epoch = std::chrono::steady_clock::time_point{};
    if (high_refresh_end_time_ != epoch && now > high_refresh_end_time_) {
        high_refresh_end_time_ = epoch;
    }
    delay_main_thread(false);
    return true;
}

void ClusterTopologyMonitor::handle_ignore_topology_timing() {
    std::chrono::steady_clock::time_point now = std::chrono::high_resolution_clock::now();
    std::chrono::steady_clock::time_point epoch = std::chrono::steady_clock::time_point{};
    std::chrono::steady_clock::time_point ignore_topology = ignore_topology_request_end_ns_.load();
    if (ignore_topology != epoch && now > ignore_topology) {
        ignore_topology_request_end_ns_.store(epoch);
    }
}

void ClusterTopologyMonitor::init_node_monitors() {
    node_threads_stop_.store(false);
    dbc_clean_up(node_threads_writer_hdbc_);
    dbc_clean_up(node_threads_reader_hdbc_);
    node_threads_writer_host_info_ = nullptr;
    node_threads_latest_topology_ = nullptr;

    std::vector<HostInfo> hosts = topology_map_->get(cluster_id_);
    if (hosts.empty()) {
        hosts = open_any_conn_get_hosts();
    }

    if (!hosts.empty() && !is_writer_connection_.load()) {
        auto end = node_monitoring_threads_.end();
        for (const HostInfo& hi : hosts) {
            std::string host_id = hi.get_host();
            if (node_monitoring_threads_.find(host_id) == end) {
                node_monitoring_threads_[host_id] =
                    std::make_shared<NodeMonitoringThread>(this, std::make_shared<HostInfo>(hi), main_writer_host_info_);
            }
        }
    }
}

bool ClusterTopologyMonitor::get_possible_writer_conn() {
    std::lock_guard<std::mutex> node_lock(node_threads_writer_hdbc_mutex_);
    std::lock_guard<std::mutex> hostinfo_lock(node_threads_writer_host_info_mutex_);
    auto* local_hdbc = reinterpret_cast<SQLHDBC>(*node_threads_writer_hdbc_.get());
    HostInfo local_hostinfo = node_threads_writer_host_info_ ? *node_threads_writer_host_info_ : HostInfo("", 0, DOWN, false, nullptr);
    if (odbc_helper_->CheckConnection(local_hdbc) && local_hostinfo.is_host_up()) {
        std::lock_guard<std::mutex> hdbc_lock(hdbc_mutex_);
        dbc_clean_up(main_hdbc_);
        main_hdbc_ = std::make_shared<SQLHDBC>(local_hdbc);
        main_writer_host_info_ = std::make_shared<HostInfo>(local_hostinfo);
        is_writer_connection_.store(true);
        high_refresh_end_time_ = std::chrono::high_resolution_clock::now() +
            high_refresh_rate_after_panic_;

        // Writer verified at initial connection & failovers but want to ignore new topology requests after failover
        // The first writer will be able to set from epoch to a proper end time
        std::chrono::steady_clock::time_point expected = std::chrono::steady_clock::time_point{}; // Epoch
        std::chrono::steady_clock::time_point new_time = std::chrono::high_resolution_clock::now() +
            std::chrono::nanoseconds(ignore_topology_request_ns_);
        ignore_topology_request_end_ns_.compare_exchange_strong(expected, new_time);

        node_threads_stop_.store(true);
        node_monitoring_threads_.clear();
        return false;
    } else {
        std::vector<HostInfo> local_topology;
        {
            std::lock_guard<std::mutex> topology_lock(node_threads_latest_topology_mutex_);
            local_topology = node_threads_latest_topology_ ? *node_threads_latest_topology_ : std::vector<HostInfo>();
        }
        auto end = node_monitoring_threads_.end();
        for (const HostInfo& hi : local_topology) {
            std::string host_id = hi.get_host();
            if (node_monitoring_threads_.find(host_id) == end) {
                node_monitoring_threads_[host_id] =
                    std::make_shared<NodeMonitoringThread>(this, std::make_shared<HostInfo>(hi), main_writer_host_info_);
            }
        }
        return true;
    }
}

ClusterTopologyMonitor::NodeMonitoringThread::NodeMonitoringThread(ClusterTopologyMonitor* monitor, const std::shared_ptr<HostInfo>& host_info, const std::shared_ptr<HostInfo>& writer_host_info) {
    this->main_monitor_ = monitor;
    this->host_info_ = host_info;
    this->writer_host_info_ = writer_host_info;
    SQLAllocHandle(SQL_HANDLE_DBC, main_monitor_->henv_, &hdbc_);
    node_thread_ = std::make_shared<std::thread>(&NodeMonitoringThread::run, this);
}

ClusterTopologyMonitor::NodeMonitoringThread::~NodeMonitoringThread() {
    if (node_thread_) {
        if (node_thread_->joinable()) {
            node_thread_->join();
        }
    }
    node_thread_ = nullptr;

    SQLDisconnect(hdbc_);
    main_monitor_->odbc_helper_->Cleanup(SQL_NULL_HANDLE, hdbc_, SQL_NULL_HANDLE);
}

void ClusterTopologyMonitor::NodeMonitoringThread::run() {
    bool thread_update_topology = false;
    std::string thread_host = host_info_->get_host();
    std::string host_endpoint = host_info_ ?
        main_monitor_->query_helper_->get_endpoint(AS_SQLTCHAR(thread_host.c_str())) : "";
    auto updated_conn_str = main_monitor_->conn_for_host(host_endpoint);
    SQLTCHAR* conn_cstr = AS_SQLTCHAR(updated_conn_str.c_str());

    try {
        bool should_stop = main_monitor_->node_threads_stop_.load();
        while (!should_stop) {
            if (!main_monitor_->odbc_helper_->CheckConnection(hdbc_)) {
                LOG(WARNING) << "Failover Monitor for: " << host_endpoint << " not connected. Trying to reconnect";
                handle_reconnect(conn_cstr);
            } else {
                // Get Writer ID
                std::string writer_id = main_monitor_->query_helper_->get_writer_id(hdbc_);
                if (!writer_id.empty()) { // Connected to a Writer
                    handle_writer_conn();
                } else { // Connected to a Reader
                    handle_reader_conn(thread_update_topology);
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(THREAD_SLEEP_MS_));
            should_stop = main_monitor_->node_threads_stop_.load();
        }
    } catch (const std::exception& ex) {
        LOG(ERROR) << "Exception while node monitoring for: " << host_endpoint << ex.what();
    }

    // Close any open connections / handles
    SQLDisconnect(hdbc_);
    main_monitor_->odbc_helper_->Cleanup(SQL_NULL_HANDLE, hdbc_, SQL_NULL_HANDLE);
}

void ClusterTopologyMonitor::NodeMonitoringThread::handle_reconnect(SQLTCHAR* conn_cstr) {
    // Disconnect
    SQLDisconnect(hdbc_);
    // Reconnect and try to query next interval
    #ifdef UNICODE
    SQLDriverConnectW(hdbc_, nullptr, conn_cstr, SQL_NTS,
        nullptr, 0, nullptr, SQL_DRIVER_NOPROMPT);
    #else
    SQLDriverConnect(hdbc_, nullptr, conn_cstr, SQL_NTS,
        nullptr, 0, nullptr, SQL_DRIVER_NOPROMPT);
    #endif
}

void ClusterTopologyMonitor::NodeMonitoringThread::handle_writer_conn() {
    std::lock_guard<std::mutex> hdbc_lock(main_monitor_->node_threads_writer_hdbc_mutex_);
    if (main_monitor_->node_threads_writer_hdbc_ != nullptr) {
        // Writer connection already set
        // Disconnect this thread's connection
        SQLDisconnect(hdbc_);
        main_monitor_->odbc_helper_->Cleanup(SQL_NULL_HANDLE, hdbc_, SQL_NULL_HANDLE);
    } else {
        // Main monitor now tracks this connection
        main_monitor_->node_threads_writer_hdbc_ = std::make_shared<SQLHDBC>(&hdbc_);
        // Update topology using writer connection
        main_monitor_->fetch_topology_update_cache(hdbc_);
        // Stop all other threads
        main_monitor_->node_threads_stop_.store(true);
        {
            std::lock_guard<std::mutex> host_info_lock(main_monitor_->node_threads_writer_host_info_mutex_);
            main_monitor_->node_threads_writer_host_info_ = host_info_;
        }
        // Prevent node monitoring from closing connection
        hdbc_ = SQL_NULL_HANDLE;
        // Writer set, stop node threads
        main_monitor_->node_threads_stop_.store(true);
    }
}

void ClusterTopologyMonitor::NodeMonitoringThread::handle_reader_conn(bool& thread_update_topology) {
    if (main_monitor_->node_threads_writer_hdbc_) {
        // Writer already set, no need for reader to update topology
        return;
    }

    // Check if this thread is updating topology
    // If it isn't check if there is already another reader
    if (thread_update_topology) {
        reader_thread_fetch_topology();
    } else {
        std::lock_guard<std::mutex> reader_hdbc_lock(main_monitor_->node_threads_reader_hdbc_mutex_);
        if (!main_monitor_->node_threads_reader_hdbc_) {
            main_monitor_->node_threads_reader_hdbc_ = std::make_shared<SQLHDBC>(&hdbc_);
            thread_update_topology = true;
            reader_thread_fetch_topology();
        }
    }
}

void ClusterTopologyMonitor::NodeMonitoringThread::reader_thread_fetch_topology() {
    // Check connection
    if (!main_monitor_->odbc_helper_->CheckConnection(hdbc_)) {
        return;
    };
    // Query for hosts
    std::vector<HostInfo> hosts;
    hosts = main_monitor_->query_helper_->query_topology(hdbc_);

    // Share / update topology to main monitor
    {
        std::lock_guard<std::mutex> lock(main_monitor_->node_threads_latest_topology_mutex_);
        main_monitor_->node_threads_latest_topology_ = std::make_shared<std::vector<HostInfo>>(hosts);
    }

    // Update cache if writer changed
    if (writer_changed_) {
        main_monitor_->update_topology_cache(hosts);
    }

    // Check if writer changed
    for (const HostInfo& hi : hosts) {
        if (hi.is_host_writer() && hi.get_host_port_pair() == writer_host_info_->get_host_port_pair()) {
            writer_changed_ = true;
            main_monitor_->update_topology_cache(hosts);
            break;
        }
    }
}
