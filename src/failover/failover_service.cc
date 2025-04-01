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

#include "failover_service.h"

#include "../util/rds_utils.h"
#include "../util/string_helper.h"
#include "cluster_topology_info.h"

std::unordered_map<std::string, std::shared_ptr<FailoverServiceTracker>> FailoverServiceTrackerHandler::global_failover_services;
std::mutex FailoverServiceTrackerHandler::map_mutex_;
static std::shared_ptr<SlidingCacheMap<std::string, std::vector<HostInfo>>> global_topology_map =
    std::make_shared<SlidingCacheMap<std::string, std::vector<HostInfo>>>();

const uint32_t FailoverService::DEFAULT_IGNORE_TOPOLOGY_REQUEST_MS =
    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::seconds(30)).count();
const uint32_t FailoverService::DEFAULT_HIGH_REFRESH_RATE_MS =
    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::seconds(10)).count();
const uint32_t FailoverService::DEFAULT_REFRESH_RATE_MS =
    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::seconds(30)).count();
const uint32_t FailoverService::DEFAULT_FAILOVER_TIMEOUT_MS =
    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::seconds(30)).count();

void FailoverServiceTrackerHandler::PutIfAbsent(std::string key, const std::shared_ptr<FailoverServiceTracker>& tracker) {
    std::lock_guard lock(map_mutex_);
    if (!global_failover_services.contains(key)) {
        global_failover_services[key] = tracker;
        LOG(INFO) << "[Failover Service] created for: " << key;
    }
}

void FailoverServiceTrackerHandler::Increment(std::string cluster_id) {
    std::lock_guard lock(map_mutex_);
    std::shared_ptr<FailoverServiceTracker> tracker = global_failover_services.at(cluster_id);
    tracker->reference_count.fetch_add(1);
    LOG(INFO) << "[Failover Service] additional reference for: " << cluster_id << ". Now at: " << tracker->reference_count;
 }

void FailoverServiceTrackerHandler::Decrement(std::string cluster_id) {
     std::lock_guard lock(map_mutex_);
     std::shared_ptr<FailoverServiceTracker> tracker = global_failover_services.at(cluster_id);
     if (tracker->reference_count > 0) {
         tracker->reference_count.fetch_sub(1);
         LOG(INFO) << "[Failover Service] removing reference for: " << cluster_id << ". Now at: " << tracker->reference_count;
         if (tracker->reference_count <= 0 && tracker->failover_inprogress.load() <= 0) {
             LOG(INFO) << "[Failover Service] ended for: " << cluster_id;
             tracker->service.reset();
         }
     }
 }

bool FailoverServiceTrackerHandler::Contains(const std::string& cluster_id) {
     std::lock_guard lock(map_mutex_);
    return global_failover_services.contains(cluster_id);
 }

std::shared_ptr<FailoverServiceTracker> FailoverServiceTrackerHandler::Get(const std::string& cluster_id) {
     std::lock_guard lock(map_mutex_);
     return global_failover_services.at(cluster_id);
 }

template <typename T, class U>
static U parse_num(const T& num_to_parse, const U& default_num) {
    U ret = default_num;
    try {
        ret = std::stoull(num_to_parse);
    } catch (const std::invalid_argument& ex) {
        LOG(WARNING) << "Could not be parsed as a number, returning the default." << ex.what();
    } catch (const std::out_of_range& ex) {
        LOG(WARNING) << "Number was out of range, returning the default." << ex.what();
    }
    return ret;
}

#ifdef UNICODE
FailoverService::FailoverService(const std::string& host, const std::string& cluster_id, std::shared_ptr<Dialect> dialect,
                                 std::shared_ptr<std::map<std::wstring, std::wstring>> conn_info,
                                 std::shared_ptr<SlidingCacheMap<std::string, std::vector<HostInfo>>> topology_map,
                                 const std::shared_ptr<ClusterTopologyMonitor>& topology_monitor,
                                 const std::shared_ptr<IOdbcHelper>& odbc_helper)
    : cluster_id_{ std::move(cluster_id) },
      dialect_{ std::move(dialect) },
      conn_info_{ std::move(conn_info) },
      topology_map_{ std::move(topology_map) },
      topology_monitor_{ std::move(topology_monitor) },
      odbc_helper_{ std::move(odbc_helper) } {
    this->init_failover_mode(host);
    this->host_selector_ = get_reader_host_selector();
    failover_timeout_ = parse_num(conn_info_->find(FAILOVER_TIMEOUT_KEY) != conn_info_->end() ?
        conn_info_->at(FAILOVER_TIMEOUT_KEY) : TEXT(""), DEFAULT_FAILOVER_TIMEOUT_MS);
    topology_monitor_->StartMonitor();
    curr_host_ = HostInfo(host, dialect_->GetDefaultPort(), UP, false, nullptr, 0);
}
#else
FailoverService::FailoverService(const std::string& host, const std::string& cluster_id, std::shared_ptr<Dialect> dialect,
                                 std::shared_ptr<std::map<std::string, std::string>> conn_info,
                                 std::shared_ptr<SlidingCacheMap<std::string, std::vector<HostInfo>>> topology_map,
                                 const std::shared_ptr<ClusterTopologyMonitor>& topology_monitor,
                                 const std::shared_ptr<IOdbcHelper>& odbc_helper)
    : cluster_id_{ std::move(cluster_id) },
      dialect_{ std::move(dialect) },
      conn_info_{ std::move(conn_info) },
      topology_map_{ std::move(topology_map) },
      topology_monitor_{ std::move(topology_monitor) },
      odbc_helper_{ std::move(odbc_helper) } {
    this->init_failover_mode(host);
    this->host_selector_ = get_reader_host_selector();
    failover_timeout_ = parse_num(conn_info_->find(FAILOVER_TIMEOUT_KEY) != conn_info_->end() ?
        conn_info_->at(FAILOVER_TIMEOUT_KEY) : TEXT(""), DEFAULT_FAILOVER_TIMEOUT_MS);
    topology_monitor_->StartMonitor();
    curr_host_ = HostInfo(host, dialect_->GetDefaultPort(), UP, false, nullptr, 0);
}
#endif

FailoverService::~FailoverService() {
    host_selector_ = nullptr;
    topology_monitor_ = nullptr;
    topology_map_ = nullptr;
}

void FailoverService::init_failover_mode(const std::string& host) {
    if (this->conn_info_->find(FAILOVER_MODE_KEY) != this->conn_info_->end()) {
#ifdef UNICODE
        std::string mode = StringHelper::ToString(this->conn_info_->at(FAILOVER_MODE_KEY));
#else
        std::string mode = this->conn_info_->at(FAILOVER_MODE_KEY);
#endif
        for (auto failover_mode_mapping : FAILOVER_MODE_MAPPING) {
            if (strcmp_case_insensitive(mode.c_str(), failover_mode_mapping.mode_str) == 0) {
                this->failover_mode_ = failover_mode_mapping.mode;
                break;
            }
        }
    }

    if (this->failover_mode_ == UNKNOWN_FAILOVER_MODE) {
        this->failover_mode_ = RdsUtils::IsRdsReaderClusterDns(host) ? READER_OR_WRITER : STRICT_WRITER;
    }
}

std::shared_ptr<HostSelector> FailoverService::get_reader_host_selector() const {
    ReaderHostSelectorStrategy selector_strategy = RANDOM;
    if (this->conn_info_->find(READER_HOST_SELECTOR_STRATEGY_KEY) != this->conn_info_->end()) {
#ifdef UNICODE
        std::string strategy = StringHelper::ToString(this->conn_info_->at(READER_HOST_SELECTOR_STRATEGY_KEY));
#else
        std::string strategy = this->conn_info_->at(FAILOVER_MODE_KEY);
#endif
        for (auto reader_host_selector_strategies_mapping : READER_HOST_SELECTOR_STRATEGIES_MAPPING) {
            if (strcmp_case_insensitive(strategy.c_str(), reader_host_selector_strategies_mapping.mode_str) == 0) {
                selector_strategy = reader_host_selector_strategies_mapping.mode;
                break;
            }
        }
    }

    switch (selector_strategy) {
        case ROUND_ROBIN:
            return std::make_shared<RoundRobinHostSelector>();
        case HIGHEST_WEIGHT:
            return std::make_shared<HighestWeightHostSelector>();
        case RANDOM:
        case UNKNOWN_STRATEGY:
        default:
            return std::make_shared<RandomHostSelector>();
    }
}

FailoverStatus FailoverService::Failover(SQLHDBC hdbc, const char* sql_state) {
    if (!check_should_failover(sql_state)) {
        LOG(WARNING) << "[Failover Service] SQL State: " << sql_state << " not supported for Failover.";
        return FAILOVER_SKIPPED;
    }

    conn_info_->insert_or_assign(ENABLE_FAILOVER_KEY, BOOL_TRUE);
    bool failover_result = false;
    if (failover_mode_ == STRICT_WRITER) {
        failover_result = failover_writer(hdbc);
    } else {
        failover_result = failover_reader(hdbc);
    }

    return failover_result ? FAILOVER_SUCCEED : FAILOVER_FAILED;
}

HostInfo FailoverService::GetCurrentHost() {
    return curr_host_;
}

bool FailoverService::check_should_failover(const char* sql_state) {
    // Check if the SQL State is related to a communication error
    const char* start = "08";
    LOG(INFO) << "Checking if SQLSTATE [" << sql_state << "] should trigger failover.";
    return strncmp(start, sql_state, strlen(start)) == 0;
}

void FailoverService::remove_candidate(const std::string& host, std::vector<HostInfo>& candidates) {
    candidates.erase(std::remove_if(candidates.begin(), candidates.end(), [host](HostInfo const& h) { return h.GetHost() == host; }),
                     candidates.end());
}

bool FailoverService::failover_reader(SQLHDBC hdbc) {
    auto get_current = [] {
        return std::chrono::steady_clock::time_point(std::chrono::high_resolution_clock::now().time_since_epoch());
    };
    auto curr_time = get_current();
    auto end = curr_time + std::chrono::milliseconds(failover_timeout_);

    LOG(INFO) << "Starting reader failover procedure.";
    // When we pass a timeout of 0, we inform the plugin service that it should update its topology without waiting
    // for it to get updated, since we do not need updated topology to establish a reader connection.
    topology_monitor_->ForceRefresh(false, 0);

    // The roles in this list might not be accurate, depending on whether the new topology has become available yet.
    std::vector<HostInfo> hosts = topology_map_->Get(cluster_id_);
    if (hosts.empty()) {
        LOG(INFO) << "No topology available.";
        return false;
    }

    std::vector<HostInfo> reader_candidates;
    HostInfo original_writer;

    for (const auto& host : hosts) {
        if (host.IsHostWriter()) {
            original_writer = host;
        } else {
            reader_candidates.push_back(host);
        }
    }

    std::unordered_map<std::string, std::string> properties;
    RoundRobinHostSelector::SetRoundRobinWeight(reader_candidates, properties);

    std::string host_string;
    bool is_original_writer_still_writer = false;
    do {
        std::vector<HostInfo> remaining_readers(reader_candidates);
        while (!remaining_readers.empty() && (curr_time = get_current()) < end) {
            LOG(INFO) << "Failover for ClusterId: " << cluster_id_ << ". Remaining Hosts: " << ClusterTopologyInfo::LogTopology(remaining_readers);
            HostInfo host;
            try {
                host = host_selector_->GetHost(remaining_readers, false, properties);
                host_string = host.GetHost();
                LOG(INFO) << "[Failover Service] Selected Host: " << host_string;
            } catch (const std::exception& e) {
                LOG(INFO) << "[Failover Service] no hosts in topology for: " << cluster_id_;
                return false;
            }
            bool is_connected = connect_to_host(hdbc, host_string);
            if (!is_connected) {
                LOG(INFO) << "[Failover Service] unable to connect to: " << host_string;
                remove_candidate(host_string, remaining_readers);
                continue;
            }

            bool is_reader = false;
            if (odbc_helper_->CheckConnection(hdbc)) {
                is_reader = is_connected_to_reader(hdbc);
                if (is_reader || (this->failover_mode_ != STRICT_READER)) {
                    LOG(INFO) << "[Failover Service] connected to a new reader for: " << host_string;
                    curr_host_ = host;
                    return true;
                }
                LOG(INFO) << "[Failover Service] Strict Reader Mode, not connected to a reader: " << host_string;
            }
            remove_candidate(host_string, remaining_readers);
            SQLDisconnect(hdbc);
            LOG(INFO) << "[Failover Service] Cleaned up first connection, required a strict reader: " << host_string << ", " << hdbc;

            if (!is_reader) {
                // The reader candidate is actually a writer, which is not valid when failoverMode is STRICT_READER.
                // We will remove it from the list of reader candidates to avoid retrying it in future iterations.
                remove_candidate(host_string, reader_candidates);
            }
        }

        // We were not able to connect to any of the original readers. We will try connecting to the original writer,
        // which may have been demoted to a reader.

        if (get_current() > end) {
            // Timed out.
            continue;
        }

        if (this->failover_mode_ == STRICT_READER && is_original_writer_still_writer) {
            // The original writer has been verified, so it is not valid when in STRICT_READER mode.
            continue;
        }

        // Try the original writer, which may have been demoted to a reader.
        host_string = original_writer.GetHost();
        bool is_connected = connect_to_host(hdbc, host_string);
        if (is_connected) {
            if (!odbc_helper_->CheckConnection(hdbc)) {
                SQLDisconnect(hdbc);
                continue;
            }
            if (is_connected_to_reader(hdbc) || failover_mode_ != STRICT_READER) {
                LOG(INFO) << "[Failover Service] reader failover connected to writer instance for: " << host_string;
                curr_host_ = original_writer;
                return true;
            }
        } else {
            LOG(INFO) << "[Failover Service] Failed to connect to host: " << original_writer;
        }

    } while (get_current() < end);

    // Timed out.
    SQLDisconnect(hdbc);
    LOG(INFO) << "[Failover Service] The reader failover process was not able to establish a connection before timing out.";
    return false;
}

bool FailoverService::failover_writer(SQLHDBC hdbc) {
    topology_monitor_->ForceRefresh(true, failover_timeout_);

    // Try connecting to a writer
    std::vector<HostInfo> hosts = topology_map_->Get(cluster_id_);
    std::unordered_map<std::string, std::string> properties;
    RoundRobinHostSelector::SetRoundRobinWeight(hosts, properties);
    HostInfo host;    
    try {
        host = host_selector_->GetHost(hosts, true, properties);
    } catch (const std::exception& e) {
        LOG(INFO) << "[Failover Service] no hosts in topology for: " << cluster_id_;
        return false;
    }
    std::string host_string = host.GetHost();
    LOG(INFO) << "[Failover Service] writer failover connection to a new writer: " << host_string;

    bool is_connected = connect_to_host(hdbc, host_string);
    if (!is_connected) {
        LOG(INFO) << "[Failover Service] writer failover unable to connect to any instance for: " << cluster_id_;
        return false;
    }
    if (odbc_helper_->CheckConnection(hdbc)) {
        if (!is_connected_to_reader(hdbc)) {
            LOG(INFO) << "[Failover Service] writer failover connected to a new writer for: " << host_string;
            curr_host_ = host;
            return true;
        }
        LOG(ERROR) << "The new writer was identified to be " << host_string << ", but querying the instance for its role returned a reader.";
        return false;
    }
    LOG(INFO) << "[Failover Service] writer failover unable to connect to any instance for: " << cluster_id_;
    SQLDisconnect(hdbc);
    return false;
}

bool FailoverService::connect_to_host(SQLHDBC hdbc, const std::string& host_string) {
    LOG(INFO) << "Attempting to connect to host: " << host_string;
#ifdef UNICODE
    conn_info_->insert_or_assign(SERVER_HOST_KEY, StringHelper::ToWstring(host_string));
    SQLSTR conn_str = ConnectionStringHelper::BuildConnectionStringW(*conn_info_);
#else
    conn_info_->insert_or_assign(SERVER_HOST_KEY, host_string);
    SQLSTR conn_str = ConnectionStringHelper::BuildConnectionString(*conn_info_);
#endif

    return odbc_helper_->ConnStrConnect(AS_SQLTCHAR(conn_str.c_str()), hdbc);
}

bool FailoverService::is_connected_to_reader(SQLHDBC hdbc) {
    if (SQL_NULL_HDBC == hdbc) {
        LOG(WARNING) << "[Failover Service] null HDBC passed to reader check.";
        return false;
    }

    SQLHSTMT stmt = SQL_NULL_HANDLE;
    bool is_reader = false;

    if (!odbc_helper_->AllocateHandle(SQL_HANDLE_STMT, hdbc, stmt, "[Failover Service] reader check failed to allocate handle")) {
        return false;
    }

    if (!odbc_helper_->ExecuteQuery(stmt, AS_SQLTCHAR(dialect_->GetIsReaderQuery().c_str()),
                                  "[Failover Service] reader check failed to execute topology query")) {
        return false;
    }

    SQLLEN rt = 0;
    SQLRETURN rc = SQLBindCol(stmt, 1, SQL_BIT, &is_reader, sizeof(is_reader), &rt);
    if (!OdbcHelper::CheckResult(rc, "[Failover Service] reader check failed to bind is_reader column", stmt, SQL_HANDLE_STMT)) {
        OdbcHelper::Cleanup(SQL_NULL_HANDLE, SQL_NULL_HANDLE, stmt);
        return false;
    }

    if (!odbc_helper_->FetchResults(stmt, "[Failover Service] failed to fetch if is_reader from results")) {
        return false;
    }

    LOG(INFO) << "[Failover Service] check reader queried: " << is_reader;
    OdbcHelper::Cleanup(SQL_NULL_HANDLE, SQL_NULL_HANDLE, stmt);
    return is_reader;
}

bool FailoverService::is_connected_to_writer(SQLHDBC hdbc) {
    SQLHSTMT stmt = SQL_NULL_HANDLE;
    SQLTCHAR writer_id[BUFFER_SIZE];

    if (!odbc_helper_->AllocateHandle(SQL_HANDLE_STMT, hdbc, stmt, "[Failover Service] writer failed to allocate handle")) {
        return false;
    }

    if (!odbc_helper_->ExecuteQuery(stmt, AS_SQLTCHAR(dialect_->GetWriterIdQuery().c_str()),
                                  "[Failover Service] writer failed to execute writer query")) {
        return false;
    }

    SQLLEN rt = 0;
    SQLRETURN rc = SQLBindCol(stmt, 1, SQL_C_SLONG, &writer_id, sizeof(writer_id), &rt);
    if (!OdbcHelper::CheckResult(rc, "[Failover Service] writer failed to bind writer_id column", stmt, SQL_HANDLE_STMT)) {
        OdbcHelper::Cleanup(SQL_NULL_HANDLE, SQL_NULL_HANDLE, stmt);
        return false;
    }

    if (!odbc_helper_->FetchResults(stmt, "[Failover Service] writer failed to fetch writer from results")) {
        return false;
    }

    return writer_id[0] != TEXT('\0');
}

bool StartFailoverService(char* service_id_c_str, DatabaseDialect dialect, const SQLTCHAR* conn_cstr) {
    std::string cluster_id(service_id_c_str);
    std::shared_ptr<Dialect> dialect_obj;
    switch (dialect) {
        case AURORA_POSTGRES:
            dialect_obj = std::make_shared<DialectAuroraPostgres>();
            break;
        default:
            LOG(ERROR) << "[Failover Service] for: " << cluster_id << ", does not accept this dialect.";
            return false;
    }

        // TODO(karezche): simplify this to reduce code duplication
#ifdef UNICODE
    std::map<std::wstring, std::wstring> conn_info;
    ConnectionStringHelper::ParseConnectionStringW(AS_CONST_WCHAR(conn_cstr), conn_info);
    // Simple wide to narrow conversion for endpoint template
    // RDS endpoints are alphanumeric only
    std::wstring wide_endpoint_template = conn_info[ENDPOINT_TEMPLATE_KEY];
    std::string endpoint_template = StringHelper::ToString(wide_endpoint_template);
    std::string host = StringHelper::ToString(conn_info[SERVER_HOST_KEY]);
    std::shared_ptr<std::map<std::wstring, std::wstring>> conn_info_ptr = std::make_shared<std::map<std::wstring, std::wstring>>(conn_info);
    conn_info[ENABLE_FAILOVER_KEY] = BOOL_FALSE;
    std::wstring updated_conn_str = ConnectionStringHelper::BuildConnectionStringW(conn_info);
#else
    std::map<std::string, std::string> conn_info;
    ConnectionStringHelper::ParseConnectionString(AS_CONST_CHAR(conn_cstr), conn_info);
    std::string endpoint_template = conn_info[ENDPOINT_TEMPLATE_KEY];
    std::string host = (conn_info[SERVER_HOST_KEY]);
    std::shared_ptr<std::map<std::string, std::string>> conn_info_ptr = std::make_shared<std::map<std::string, std::string>>(conn_info);
    conn_info[ENABLE_FAILOVER_KEY] = BOOL_FALSE;
    std::string updated_conn_str = ConnectionStringHelper::BuildConnectionString(conn_info);
#endif

    if (endpoint_template.empty()) {
        endpoint_template = RdsUtils::GetRdsInstanceHostPattern(host);
    }

    if (cluster_id.empty()) {
        cluster_id = RdsUtils::GetRdsClusterId(host);
    #ifdef UNICODE
        conn_info_ptr->insert_or_assign(CLUSTER_ID_KEY, StringHelper::ToWstring(cluster_id));
    #else
        conn_info_ptr->insert_or_assign(CLUSTER_ID_KEY, cluster_id);
    #endif
        // If the original input was empty, copy the generated ID back to caller
        strncpy(service_id_c_str, cluster_id.c_str(), MAX_CLUSTER_ID_LEN);
        LOG(INFO) << "[Failover Service] Generated ClusterId from host: " << cluster_id;
    }

    std::shared_ptr<FailoverServiceTracker> tracker;
    try {
        uint32_t ignore_topology_request_ms = parse_num(conn_info[IGNORE_TOPOLOGY_REQUEST_KEY], FailoverService::DEFAULT_IGNORE_TOPOLOGY_REQUEST_MS);
        uint32_t high_refresh_rate_ms = parse_num(conn_info[HIGH_REFRESH_RATE_KEY], FailoverService::DEFAULT_HIGH_REFRESH_RATE_MS);

        uint32_t refresh_rate_ms = parse_num(conn_info[REFRESH_RATE_KEY], FailoverService::DEFAULT_REFRESH_RATE_MS);

        if (!FailoverServiceTrackerHandler::Contains(cluster_id)) {
            tracker = std::make_shared<FailoverServiceTracker>();
            tracker->reference_count = 1;
            tracker->service = std::make_shared<FailoverService>(
                host, cluster_id, dialect_obj, conn_info_ptr, global_topology_map,
                std::make_shared<ClusterTopologyMonitor>(
                    cluster_id, global_topology_map, AS_SQLTCHAR(updated_conn_str.c_str()), std::make_shared<OdbcHelperWrapper>(),
                    std::make_shared<ClusterTopologyQueryHelper>(dialect_obj->GetDefaultPort(), endpoint_template,
                                                                 dialect_obj->GetTopologyQuery(), dialect_obj->GetWriterIdQuery(),
                                                                 dialect_obj->GetNodeIdQuery()),
                    ignore_topology_request_ms, high_refresh_rate_ms, refresh_rate_ms),
                std::make_shared<OdbcHelperWrapper>());

            // Check again to see if the other thread has set service tracker for cluster id
            // If still empty, put new tracker. Let tracker descope and free itself otherwise
            FailoverServiceTrackerHandler::PutIfAbsent(cluster_id, tracker);
        } else {
            FailoverServiceTrackerHandler::Increment(cluster_id);
        }
    } catch (const std::exception& ex) {
        LOG(ERROR) << "Failed to create Failover Service for: " << cluster_id << ". " << ex.what();
        return false;
    }
    return true;
}

void StopFailoverService(const char* service_id_c_str) {
    std::string cluster_id(service_id_c_str);
    if (!FailoverServiceTrackerHandler::Contains(cluster_id)) {
        LOG(INFO) << "[Failover Service] not found for: " << cluster_id;
        return;
    }

    FailoverServiceTrackerHandler::Decrement(cluster_id);
}

FailoverResult FailoverConnection(const char* service_id_c_str, const char* sql_state, SQLHENV henv) {
    std::string cluster_id(service_id_c_str);
    if (!FailoverServiceTrackerHandler::Contains(cluster_id)) {
        LOG(INFO) << "[Failover Service] no tracker found for: " << cluster_id << ". Cannot perform failover.";
        return FailoverResult{ .status = FAILOVER_FAILED, .hdbc = SQL_NULL_HDBC };
    }
    std::shared_ptr<FailoverServiceTracker> tracker = FailoverServiceTrackerHandler::Get(cluster_id);
    if (nullptr == tracker->service) {
        LOG(INFO) << "[Failover Service] no active Failover Service: " << cluster_id << ". Cannot perform failover.";
        return FailoverResult{ .status = FAILOVER_FAILED, .hdbc = SQL_NULL_HDBC };
    }
    // Set flag to ensure tracker is not cleaned up during failover disconnection
    tracker->failover_inprogress.fetch_add(1);
    SQLHDBC local_hdbc = SQL_NULL_HDBC;
    // open a new connection
    SQLAllocHandle(SQL_HANDLE_DBC, henv, &local_hdbc);
    FailoverStatus status = tracker->service->Failover(local_hdbc, sql_state);
    tracker->failover_inprogress.fetch_sub(1);
    if (FAILOVER_SUCCEED != status) {
        OdbcHelper::Cleanup(SQL_NULL_HENV, local_hdbc, SQL_NULL_HSTMT);
        LOG(WARNING) << "[Failover Service] Unsuccessful failover for: " << cluster_id;
    }
    return FailoverResult{ .status = status, .hdbc = local_hdbc };
}
