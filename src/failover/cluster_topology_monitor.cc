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

#include "cluster_topology_monitor.h"

#include <sqlext.h>

#include <glog/logging.h>

#include "../util/cluster_topology_helper.h"
#include "../util/connection_string_helper.h"
#include "../util/connection_string_keys.h"
#include "string_helper.h"

ClusterTopologyMonitor::ClusterTopologyMonitor(
        const std::string& cluster_id,
        const std::shared_ptr<SlidingCacheMap<std::string, std::vector<HostInfo>>>& topology_map,
        const SQLTCHAR* conn_cstr,
        const std::shared_ptr<IOdbcHelper>& odbc_helper,
        const std::shared_ptr<ClusterTopologyQueryHelper>& query_helper,
        uint32_t ignore_topology_request_ms,
        uint32_t high_refresh_rate_ms,
        uint32_t refresh_rate_ms):
        cluster_id_{ cluster_id },
        topology_map_ { topology_map },
        odbc_helper_ { std::move(odbc_helper) },
        query_helper_ { std::move(query_helper) },
        ignore_topology_request_ms_ { ignore_topology_request_ms },
        high_refresh_rate_ms_ { high_refresh_rate_ms },
        refresh_rate_ms_ { refresh_rate_ms } {

    if (!this->odbc_helper_->CheckResult(SQLAllocHandle(SQL_HANDLE_ENV, nullptr, &henv_),
        std::string("Cluster Topology Monitor unable to allocate environment handle for: ") + cluster_id,
            henv_, SQL_HANDLE_ENV)) {
        throw std::runtime_error(std::string("Cluster Topology Monitor unable to allocate HENV for ClusterId: ") + cluster_id);
    }
    conn_str_ = StringHelper::ToSQLSTR(conn_cstr);
}

ClusterTopologyMonitor::~ClusterTopologyMonitor() {
    is_running_.store(false);
    node_threads_stop_.store(true);

    // Notify node threads if they are stuck waiting
    {
        std::lock_guard lock(request_update_topology_mutex_);
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
    std::lock_guard hdbc_lock(hdbc_mutex_);
    dbc_clean_up(main_hdbc_);
    odbc_helper_->Cleanup(henv_, SQL_NULL_HDBC, SQL_NULL_HSTMT);
}

void ClusterTopologyMonitor::SetClusterId(const std::string& cluster_id) {
    this->cluster_id_ = cluster_id;
}

std::vector<HostInfo> ClusterTopologyMonitor::ForceRefresh(const bool verify_writer, const uint32_t timeout_ms) {
    std::chrono::steady_clock::time_point now =
        std::chrono::steady_clock::time_point(std::chrono::high_resolution_clock::now().time_since_epoch());
    std::chrono::steady_clock::time_point epoch = std::chrono::steady_clock::time_point{};
    std::chrono::steady_clock::time_point ignore_topology = ignore_topology_request_end_ms_.load();
    if (ignore_topology != epoch && now > ignore_topology) {
        // Previous failover has just completed. We can use results of it without triggering a new topology update.
        std::vector<HostInfo> hosts = topology_map_->Get(cluster_id_);
        if (!hosts.empty()) {
            return hosts;
        }
    }

    if (verify_writer) {
        std::lock_guard hdbc_lock(hdbc_mutex_);
        dbc_clean_up(main_hdbc_);
        is_writer_connection_.store(false);
    }

    std::vector<HostInfo> hosts = WaitForTopologyUpdate(timeout_ms);
    if (!hosts.empty()) {
        topology_map_->Put(cluster_id_, hosts);
    }
    return hosts;
}

std::vector<HostInfo> ClusterTopologyMonitor::ForceRefresh(SQLHDBC hdbc, const uint32_t timeout_ms) {
    if (is_writer_connection_) {
        // Push monitoring thread to refresh topology with a verified connection
        return WaitForTopologyUpdate(timeout_ms);
    }

    // Otherwise use provided unverified connection to update topology
    return FetchTopologyUpdateCache(hdbc);
}

void ClusterTopologyMonitor::StartMonitor() {
    if (!is_running_.load()) {
        is_running_.store(true);
        this->monitoring_thread_ = std::make_shared<std::thread>(&ClusterTopologyMonitor::Run, this);
    }
}

void ClusterTopologyMonitor::Run() {
    std::string c = StringHelper::ToString(this->conn_str_);

    try {
        LOG(INFO) << "Start cluster topology monitoring thread for " << c;
        while (is_running_.load()) {
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
        LOG(INFO) << "Stop cluster topology monitoring thread for " << c;
    } catch (const std::exception& ex) {
        LOG(ERROR) << "Cluster Topology Main Monitor encountered error: " << ex.what();
    }
}

std::vector<HostInfo> ClusterTopologyMonitor::WaitForTopologyUpdate(uint32_t timeout_ms) {
    std::vector<HostInfo> curr_hosts = topology_map_->Get(cluster_id_);
    std::vector<HostInfo> new_hosts = curr_hosts;
    {
        std::lock_guard<std::mutex> lock(request_update_topology_mutex_);
        request_update_topology_.store(true);
    }
    request_update_topology_cv_.notify_all();

    if (timeout_ms == 0) {
        LOG(INFO) << "A topology refresh was requested, but the given timeout for the request was 0ms. Returning cached hosts.";
        return curr_hosts;
    }
    std::chrono::steady_clock::time_point curr_time =
        std::chrono::steady_clock::time_point(std::chrono::high_resolution_clock::now().time_since_epoch());
    std::chrono::steady_clock::time_point end = curr_time + std::chrono::milliseconds(timeout_ms);

    std::unique_lock<std::mutex> topology_lock(topology_updated_mutex_);
    // TODO(karezche): refactor the code to compare references instead of values
    // Current implementation does not support comparing curr_hosts and new_hosts by their references.
    while (curr_time < end && curr_hosts == new_hosts) {
        LOG(INFO) << "Host reference comparison has failed, curr_hosts: " << ClusterTopologyHelper::LogTopology(curr_hosts)
                  << " new hosts: " << ClusterTopologyHelper::LogTopology(new_hosts);
        topology_updated_.wait_for(topology_lock, std::chrono::milliseconds(TOPOLOGY_UPDATE_WAIT_MS));
        new_hosts = topology_map_->Get(cluster_id_);
        curr_time = std::chrono::steady_clock::time_point(std::chrono::high_resolution_clock::now().time_since_epoch());
    }
    LOG(INFO) << "new hosts have been updated";

    if (curr_time >= end) {
        LOG(ERROR) << "Cluster Monitor topology did not update within the maximum time: " << std::to_string(timeout_ms) << "for cluster ID: " << cluster_id_;
    }

    return new_hosts;
}

void ClusterTopologyMonitor::DelayMainThread(bool use_high_refresh_rate) {
    std::chrono::steady_clock::time_point curr_time =
        std::chrono::steady_clock::time_point(std::chrono::high_resolution_clock::now().time_since_epoch());
    if ((high_refresh_end_time_ != std::chrono::steady_clock::time_point() &&
            curr_time < high_refresh_end_time_) || request_update_topology_.load()) {
        use_high_refresh_rate = true;
    }

    std::chrono::steady_clock::time_point end = curr_time + (use_high_refresh_rate ?
        std::chrono::milliseconds(high_refresh_rate_ms_) :
        std::chrono::milliseconds(refresh_rate_ms_));
    std::unique_lock<std::mutex> request_lock(request_update_topology_mutex_);
    do {
        request_update_topology_cv_.wait_for(request_lock, std::chrono::milliseconds(TOPOLOGY_REQUEST_WAIT_MS));
        curr_time = std::chrono::steady_clock::time_point(std::chrono::high_resolution_clock::now().time_since_epoch());
    } while (!request_update_topology_.load() &&
        curr_time < end && !is_running_.load()
    );
}

std::vector<HostInfo> ClusterTopologyMonitor::FetchTopologyUpdateCache(const SQLHDBC hdbc) {
    std::vector<HostInfo> hosts;
    if (!odbc_helper_->CheckConnection(hdbc)) {
        LOG(ERROR) << "Cluster Monitor invalid connection for querying for ClusterId: " << cluster_id_;
        return hosts;
    }
    hosts = query_helper_->QueryTopology(hdbc);
    if (hosts.empty()) {
        LOG(ERROR) << "Cluster Monitor queried and found no topology for ClusterId: " << cluster_id_;
    } else {
        // Update if new topology is found
        UpdateTopologyCache(hosts);
    }

    return hosts;
}

void ClusterTopologyMonitor::UpdateTopologyCache(const std::vector<HostInfo>& hosts) {
    std::unique_lock<std::mutex> request_lock(request_update_topology_mutex_);
    std::unique_lock<std::mutex> update_lock(topology_updated_mutex_);

    // Update topology and notify threads
    topology_map_->Put(cluster_id_, hosts);
    request_update_topology_.store(false);
    topology_updated_.notify_all();
    request_update_topology_cv_.notify_one();
}

SQLSTR ClusterTopologyMonitor::ConnForHost(const std::string& new_host) {
    SQLSTR new_host_str = StringHelper::ToSQLSTR(new_host);
    std::map<SQLSTR, SQLSTR> conn_map;
    ConnectionStringHelper::ParseConnectionString(conn_str_, conn_map);

    if (conn_map.find(SERVER_HOST_KEY) != conn_map.end()) {
        conn_map[SERVER_HOST_KEY] = new_host_str;
    }
    if (conn_map.find(ENABLE_FAILOVER_KEY) != conn_map.end()) {
        conn_map[ENABLE_FAILOVER_KEY] = BOOL_FALSE;
    }

    return ConnectionStringHelper::BuildConnectionString(conn_map);
}

bool ClusterTopologyMonitor::in_panic_mode() {
    return !main_hdbc_
        || !is_writer_connection_.load();
}

std::vector<HostInfo> ClusterTopologyMonitor::open_any_conn_get_hosts() {
    SQLRETURN rc;    
    bool thread_writer_verified = false;
    auto* conn_cstr = AS_SQLTCHAR(conn_str_.c_str());
    if (!main_hdbc_) {
        SQLHDBC local_hdbc;
        // open a new connection
        SQLSetEnvAttr(henv_, SQL_ATTR_ODBC_VERSION, reinterpret_cast<SQLPOINTER>(SQL_OV_ODBC3), 0);
        SQLAllocHandle(SQL_HANDLE_DBC, henv_, &local_hdbc);
        rc = SQLDriverConnect(local_hdbc, nullptr, conn_cstr, SQL_NTS,
            nullptr, 0, nullptr, SQL_DRIVER_NOPROMPT);
        if (!odbc_helper_->CheckResult(rc, std::string("ClusterTopologyMonitor failed to open connection for clusterId: ") + cluster_id_, local_hdbc, SQL_HANDLE_DBC)) {
            SQLDisconnect(local_hdbc);
            SQLFreeHandle(SQL_HANDLE_DBC, local_hdbc);
            return std::vector<HostInfo>();
        }
        // Check if another thread already set HDBC
        std::lock_guard<std::mutex> hdbc_lock(hdbc_mutex_);
        if (!main_hdbc_) {
            main_hdbc_ = std::make_shared<SQLHDBC>(local_hdbc);
            std::string writer_id = query_helper_->GetWriterId(local_hdbc);
            if (!writer_id.empty()) {
                thread_writer_verified = true;
                is_writer_connection_.store(true);
                // TODO(yuenhcol), double lock makes this complicated & complex, need to come back to refactor
                std::lock_guard host_info_lock(node_threads_writer_hdbc_mutex_);
                main_writer_host_info_ = std::make_shared<HostInfo>(query_helper_->CreateHost(AS_SQLTCHAR(writer_id.c_str()), true, 0, 0));
            }
        } else {
            // Connection already set, close local HDBC
            SQLDisconnect(local_hdbc);
            SQLFreeHandle(SQL_HANDLE_DBC, local_hdbc);
        }
    }

    std::vector<HostInfo> hosts = FetchTopologyUpdateCache(reinterpret_cast<SQLHDBC>(*(main_hdbc_.get())));
    if (thread_writer_verified) {
        // Writer verified at initial connection & failovers but want to ignore new topology requests after failover
        // The first writer will be able to set from epoch to a proper end time
        std::chrono::steady_clock::time_point expected = std::chrono::steady_clock::time_point{}; // Epoch
        std::chrono::steady_clock::time_point new_time =
            std::chrono::steady_clock::time_point(std::chrono::high_resolution_clock::now().time_since_epoch()) +
            std::chrono::milliseconds(ignore_topology_request_ms_);
        ignore_topology_request_end_ms_.compare_exchange_strong(expected, new_time);
    }

    if (hosts.empty()) {
        // No topology, close connection
        std::lock_guard hdbc_lock(hdbc_mutex_);
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
    DelayMainThread(true);
    return should_handle_topology_timing;
}

bool ClusterTopologyMonitor::handle_regular_mode() {
    node_monitoring_threads_.clear();
    std::vector<HostInfo> hosts;
    {
        std::lock_guard hdbc_lock(hdbc_mutex_);
        hosts = FetchTopologyUpdateCache(reinterpret_cast<SQLHDBC>(main_hdbc_ ? *main_hdbc_.get() : SQL_NULL_HDBC));
    }

    // No hosts, switch to panic
    if (hosts.empty()) {
        std::lock_guard hdbc_lock(hdbc_mutex_);
        dbc_clean_up(main_hdbc_);
        is_writer_connection_.store(false);
        return false;
    }

    std::chrono::steady_clock::time_point now =
        std::chrono::steady_clock::time_point(std::chrono::high_resolution_clock::now().time_since_epoch());
    std::chrono::steady_clock::time_point epoch = std::chrono::steady_clock::time_point{};
    if (high_refresh_end_time_ != epoch && now > high_refresh_end_time_) {
        high_refresh_end_time_ = epoch;
    }

    if (high_refresh_end_time_ == epoch) {
        ClusterTopologyHelper::LogTopology(hosts);
    }
    DelayMainThread(false);
    return true;
}

void ClusterTopologyMonitor::handle_ignore_topology_timing() {
    std::chrono::steady_clock::time_point now =
        std::chrono::steady_clock::time_point(std::chrono::high_resolution_clock::now().time_since_epoch());
    std::chrono::steady_clock::time_point epoch = std::chrono::steady_clock::time_point{};
    std::chrono::steady_clock::time_point ignore_topology = ignore_topology_request_end_ms_.load();
    if (ignore_topology != epoch && now > ignore_topology) {
        ignore_topology_request_end_ms_.store(epoch);
    }
}

void ClusterTopologyMonitor::init_node_monitors() {
    node_threads_stop_.store(false);
    {
        std::lock_guard<std::mutex> writer_hdbc_lock(node_threads_writer_hdbc_mutex_);
        dbc_clean_up(node_threads_writer_hdbc_);
    }
    {
        std::lock_guard<std::mutex> reader_hdbc_lock(node_threads_reader_hdbc_mutex_);
        dbc_clean_up(node_threads_reader_hdbc_);
    }
    node_threads_writer_host_info_ = nullptr;
    node_threads_latest_topology_ = nullptr;

    std::vector<HostInfo> hosts = topology_map_->Get(cluster_id_);
    if (hosts.empty()) {
        hosts = open_any_conn_get_hosts();
    }

    if (!hosts.empty() && !is_writer_connection_.load()) {
        auto end = node_monitoring_threads_.end();
        for (const HostInfo& hi : hosts) {
            std::string host_id = hi.GetHost();
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
    auto* local_hdbc = (node_threads_writer_hdbc_ && node_threads_writer_hdbc_.get()) ?
        reinterpret_cast<SQLHDBC>(*node_threads_writer_hdbc_.get()) : SQL_NULL_HDBC;
    HostInfo local_hostinfo = node_threads_writer_host_info_ ? *node_threads_writer_host_info_ : HostInfo("", 0, DOWN, false, nullptr);
    if (SQL_NULL_HDBC != local_hdbc && odbc_helper_->CheckConnection(local_hdbc) && local_hostinfo.IsHostUp()) {
        LOG(INFO) << "The writer host detected by the node monitors was picked up by the topology monitor: " << local_hostinfo;
        std::lock_guard<std::mutex> hdbc_lock(hdbc_mutex_);
        dbc_clean_up(main_hdbc_);
        main_hdbc_ = std::move(node_threads_writer_hdbc_);
        main_writer_host_info_ = std::make_shared<HostInfo>(local_hostinfo);
        is_writer_connection_.store(true);
        high_refresh_end_time_ =
            std::chrono::steady_clock::time_point(std::chrono::high_resolution_clock::now().time_since_epoch()) +
            high_refresh_rate_after_panic_;

        // Writer verified at initial connection & failovers but want to ignore new topology requests after failover
        // The first writer will be able to set from epoch to a proper end time
        std::chrono::steady_clock::time_point expected = std::chrono::steady_clock::time_point{}; // Epoch
        std::chrono::steady_clock::time_point new_time =
            std::chrono::steady_clock::time_point(std::chrono::high_resolution_clock::now().time_since_epoch()) +
            std::chrono::milliseconds(ignore_topology_request_ms_);
        ignore_topology_request_end_ms_.compare_exchange_strong(expected, new_time);

        node_threads_stop_.store(true);
        node_monitoring_threads_.clear();
        return false;
    }
    std::vector<HostInfo> local_topology;
    {
        std::lock_guard<std::mutex> topology_lock(node_threads_latest_topology_mutex_);
        local_topology = node_threads_latest_topology_ ? *node_threads_latest_topology_ : std::vector<HostInfo>();
    }
    auto end = node_monitoring_threads_.end();
    for (const HostInfo& hi : local_topology) {
        std::string host_id = hi.GetHost();
        if (node_monitoring_threads_.find(host_id) == end) {
            node_monitoring_threads_[host_id] =
                std::make_shared<NodeMonitoringThread>(this, std::make_shared<HostInfo>(hi), main_writer_host_info_);
        }
    }
    return true;
}

ClusterTopologyMonitor::NodeMonitoringThread::NodeMonitoringThread(ClusterTopologyMonitor* monitor, const std::shared_ptr<HostInfo>& host_info, const std::shared_ptr<HostInfo>& writer_host_info) {
    this->main_monitor_ = monitor;
    this->host_info_ = host_info;
    this->writer_host_info_ = writer_host_info;
    node_thread_ = std::make_shared<std::thread>(&NodeMonitoringThread::run, this);
}

ClusterTopologyMonitor::NodeMonitoringThread::~NodeMonitoringThread() {
    if (node_thread_) {
        if (node_thread_->joinable()) {
            node_thread_->join();
        }
    }
    node_thread_ = nullptr;

    // Main thread will clean up if this Node was used as a reader connection
    if (!reader_update_topology_ && hdbc_) {
        main_monitor_->odbc_helper_->Cleanup(SQL_NULL_HANDLE, hdbc_, SQL_NULL_HANDLE);
    }
}

void ClusterTopologyMonitor::NodeMonitoringThread::run() {
    std::string thread_host = host_info_->GetHost();
    auto updated_conn_str = main_monitor_->ConnForHost(thread_host);
    SQLTCHAR* conn_cstr = AS_SQLTCHAR(updated_conn_str.c_str());

    try {
        bool should_stop = main_monitor_->node_threads_stop_.load();
        while (!should_stop) {
            if (!main_monitor_->odbc_helper_->CheckConnection(hdbc_)) {
                if (hdbc_ != SQL_NULL_HDBC) {
                    // Not an initial connection.
                    LOG(WARNING) << "Failover Monitor for: " << thread_host << " not connected. Trying to reconnect.";
                }
                handle_reconnect(conn_cstr);
            } else {
                // Get Writer ID
                std::string writer_id = main_monitor_->query_helper_->GetWriterId(hdbc_);
                if (!writer_id.empty()) {  // Connected to a Writer
                    LOG(WARNING) << "Writer " << writer_id << " detected by node monitoring thread: " << thread_host;
                    handle_writer_conn();
                } else { // Connected to a Reader
                    handle_reader_conn();
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(THREAD_SLEEP_MS_));
            should_stop = main_monitor_->node_threads_stop_.load();
        }
    } catch (const std::exception& ex) {
        LOG(ERROR) << "Exception while node monitoring for: " << thread_host << ex.what();
    }

    // Close any open connections / handles
    main_monitor_->odbc_helper_->Cleanup(SQL_NULL_HANDLE, hdbc_, SQL_NULL_HANDLE);
    hdbc_ = SQL_NULL_HDBC;
}

void ClusterTopologyMonitor::NodeMonitoringThread::handle_reconnect(SQLTCHAR* conn_cstr) {
    if (hdbc_ != SQL_NULL_HDBC) {
        // Disconnect if hdbc is not null
        main_monitor_->odbc_helper_->Cleanup(SQL_NULL_HANDLE, hdbc_, SQL_NULL_HANDLE);
    }
    // Reallocate for new connection
    SQLAllocHandle(SQL_HANDLE_DBC, main_monitor_->henv_, &hdbc_);
    // Reconnect and try to query next interval
    SQLDriverConnect(hdbc_, nullptr, conn_cstr, SQL_NTS,
        nullptr, 0, nullptr, SQL_DRIVER_NOPROMPT);
}

void ClusterTopologyMonitor::NodeMonitoringThread::handle_writer_conn() {
    std::lock_guard<std::mutex> hdbc_lock(main_monitor_->node_threads_writer_hdbc_mutex_);
    if (main_monitor_->node_threads_writer_hdbc_ != nullptr) {
        // Writer connection already set
        // Disconnect this thread's connection
        LOG(INFO) << "Writer connection already set, disconnect this thread's connection";
        main_monitor_->odbc_helper_->Cleanup(SQL_NULL_HANDLE, hdbc_, SQL_NULL_HANDLE);
    } else {
        // Main monitor now tracks this connection
        LOG(INFO) << "Main monitor now tracks this connection";
        main_monitor_->node_threads_writer_hdbc_ = std::make_shared<SQLHDBC>(hdbc_);
        // Update topology using writer connection
        LOG(INFO) << "Update topology using writer connection";
        main_monitor_->FetchTopologyUpdateCache(hdbc_);
        {
            std::lock_guard<std::mutex> host_info_lock(main_monitor_->node_threads_writer_host_info_mutex_);
            main_monitor_->node_threads_writer_host_info_ = host_info_;
        }
    }
    hdbc_ = SQL_NULL_HDBC;
    main_monitor_->node_threads_stop_.store(true);
}

void ClusterTopologyMonitor::NodeMonitoringThread::handle_reader_conn() {
    if (main_monitor_->node_threads_writer_hdbc_) {
        // Writer already set, no need for reader to update topology
        return;
    }

    // Check if this thread is updating topology
    // If it isn't check if there is already another reader
    if (reader_update_topology_) {
        reader_thread_fetch_topology();
    } else {
        std::lock_guard<std::mutex> reader_hdbc_lock(main_monitor_->node_threads_reader_hdbc_mutex_);
        if (!main_monitor_->node_threads_reader_hdbc_) {
            main_monitor_->node_threads_reader_hdbc_ = std::make_shared<SQLHDBC>(hdbc_);
            reader_update_topology_ = true;
            hdbc_ = SQL_NULL_HDBC;
            reader_thread_fetch_topology();
        }
    }
}

void ClusterTopologyMonitor::NodeMonitoringThread::reader_thread_fetch_topology() {
    auto* local_hdbc = reinterpret_cast<SQLHDBC>(*main_monitor_->node_threads_reader_hdbc_.get());
    // Check connection
    if (!main_monitor_->odbc_helper_->CheckConnection(local_hdbc)) {
        return;
    };
    // Query for hosts
    std::vector<HostInfo> hosts;
    hosts = main_monitor_->query_helper_->QueryTopology(local_hdbc);

    // Share / update topology to main monitor
    {
        std::lock_guard<std::mutex> lock(main_monitor_->node_threads_latest_topology_mutex_);
        main_monitor_->node_threads_latest_topology_ = std::make_shared<std::vector<HostInfo>>(hosts);
    }

    // Update cache if writer changed
    if (writer_changed_) {
        LOG(INFO) << "Writer has changed, updating cache";
        main_monitor_->UpdateTopologyCache(hosts);
    }

    // Check if writer changed
    for (const HostInfo& hi : hosts) {
        if (hi.IsHostWriter() && (!writer_host_info_ || hi.GetHostPortPair() == writer_host_info_->GetHostPortPair())) {
            writer_changed_ = true;
            main_monitor_->UpdateTopologyCache(hosts);
            break;
        }
    }
}
