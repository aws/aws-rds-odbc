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

#include "cluster_topology_query_helper.h"
#include "../util/string_helper.h"

#include <cmath>

ClusterTopologyQueryHelper::ClusterTopologyQueryHelper(int port, std::string endpoint_template, SQLSTR topology_query, SQLSTR writer_id_query,
                                                       SQLSTR node_id_query)
    : port{ port },
      endpoint_template_{ std::move(endpoint_template) },
      topology_query_{ std::move(topology_query) },
      writer_id_query_{ std::move(writer_id_query) },
      node_id_query_{ std::move(node_id_query) } {}

std::string ClusterTopologyQueryHelper::GetWriterId(SQLHDBC hdbc) {
    SQLRETURN rc;
    SQLHSTMT stmt = SQL_NULL_HANDLE;
    SQLTCHAR writer_id[BUFFER_SIZE] = {0};
    SQLLEN rt = 0;

    if (!OdbcHelper::AllocateHandle(SQL_HANDLE_STMT, hdbc, stmt, "ClusterTopologyQueryHelper failed to allocate handle")) {
        return std::string();
    }

    if (!OdbcHelper::ExecuteQuery(stmt, AS_SQLTCHAR(writer_id_query_.c_str()), "ClusterTopologyQueryHelper failed to execute writer query")) {
        return std::string();
    }
    
    rc = SQLBindCol(stmt, NODE_ID_COL, SQL_C_TCHAR, &writer_id, sizeof(writer_id), &rt);
    if (!OdbcHelper::CheckResult(rc, "ClusterTopologyQueryHelper failed to bind writer_id column", stmt, SQL_HANDLE_STMT)) {
        OdbcHelper::Cleanup(SQL_NULL_HANDLE, SQL_NULL_HANDLE, stmt);
        return std::string();
    }

    if (!OdbcHelper::FetchResults(stmt, "ClusterTopologyQueryHelper failed to fetch writer from results")) {
        return std::string();
    }

    OdbcHelper::Cleanup(SQL_NULL_HANDLE, SQL_NULL_HANDLE, stmt);
    return StringHelper::ToString(writer_id);
}

std::string ClusterTopologyQueryHelper::GetNodeId(SQLHDBC hdbc) {
    SQLHSTMT stmt = SQL_NULL_HANDLE;
    SQLTCHAR node_id[BUFFER_SIZE] = {0};
    SQLLEN rt = 0;

    if (!OdbcHelper::AllocateHandle(SQL_HANDLE_STMT, hdbc, stmt, "ClusterTopologyQueryHelper failed to allocate handle")) {
        return std::string();
    }

    if (!OdbcHelper::ExecuteQuery(stmt, AS_SQLTCHAR(node_id_query_.c_str()), "ClusterTopologyQueryHelper failed to execute node ID query")) {
        return std::string();
    }

    SQLRETURN rc = SQLBindCol(stmt, NODE_ID_COL, SQL_C_TCHAR, &node_id, sizeof(node_id), &rt);
    if (!OdbcHelper::CheckResult(rc, "ClusterTopologyQueryHelper failed to bind node_id column", stmt, SQL_HANDLE_STMT)) {
        OdbcHelper::Cleanup(SQL_NULL_HANDLE, SQL_NULL_HANDLE, stmt);
        return std::string();
    }

    if (!OdbcHelper::FetchResults(stmt, "ClusterTopologyQueryHelper failed to fetch node ID from results")) {
        return std::string();
    }

    OdbcHelper::Cleanup(SQL_NULL_HANDLE, SQL_NULL_HANDLE, stmt);
    return StringHelper::ToString(node_id);
}

std::vector<HostInfo> ClusterTopologyQueryHelper::QueryTopology(SQLHDBC hdbc) {
    SQLHSTMT stmt = SQL_NULL_HANDLE;
    SQLTCHAR node_id[BUFFER_SIZE] = {0};
    bool is_writer;
    SQLREAL cpu_usage;
    SQLINTEGER replica_lag_ms;
    SQLLEN rt = 0;

    if (!OdbcHelper::AllocateHandle(SQL_HANDLE_STMT, hdbc, stmt, "ClusterTopologyQueryHelper failed to allocate handle")) {
        return std::vector<HostInfo>();
    }

    if (!OdbcHelper::ExecuteQuery(stmt, AS_SQLTCHAR(topology_query_.c_str()), "ClusterTopologyQueryHelper failed to execute topology query")) {
        return std::vector<HostInfo>();
    }

    SQLRETURN rc = SQLBindCol(stmt, NODE_ID_COL, SQL_C_TCHAR, &node_id, sizeof(node_id), &rt);
    if (!OdbcHelper::CheckResult(rc, "ClusterTopologyQueryHelper failed to bind node_id column", stmt, SQL_HANDLE_STMT)) {
        OdbcHelper::Cleanup(SQL_NULL_HANDLE, SQL_NULL_HANDLE, stmt);
        return std::vector<HostInfo>();
    }

    rc = SQLBindCol(stmt, IS_WRITER_COL, SQL_BIT, &is_writer, sizeof(is_writer), &rt);
    if (!OdbcHelper::CheckResult(rc, "ClusterTopologyQueryHelper failed to bind is_writer column", stmt, SQL_HANDLE_STMT)) {
        OdbcHelper::Cleanup(SQL_NULL_HANDLE, SQL_NULL_HANDLE, stmt);
        return std::vector<HostInfo>();
    }

    rc = SQLBindCol(stmt, CPU_USAGE_COL, SQL_REAL, &cpu_usage, sizeof(cpu_usage), &rt);
    if (!OdbcHelper::CheckResult(rc, "ClusterTopologyQueryHelper failed to bind cpu_usage column", stmt, SQL_HANDLE_STMT)) {
        OdbcHelper::Cleanup(SQL_NULL_HANDLE, SQL_NULL_HANDLE, stmt);
        return std::vector<HostInfo>();
    }

    rc = SQLBindCol(stmt, REPLICA_LAG_COL, SQL_INTEGER, &replica_lag_ms, sizeof(replica_lag_ms), &rt);
    if (!OdbcHelper::CheckResult(rc, "ClusterTopologyQueryHelper failed to bind replica_lag_ms column", stmt, SQL_HANDLE_STMT)) {
        OdbcHelper::Cleanup(SQL_NULL_HANDLE, SQL_NULL_HANDLE, stmt);
        return std::vector<HostInfo>();
    }

    bool fetch_success = OdbcHelper::FetchResults(stmt, "ClusterTopologyQueryHelper failed to fetch topology from results");
    std::vector<HostInfo> hosts;
    while (fetch_success) {
        hosts.push_back(CreateHost(node_id, is_writer, cpu_usage, replica_lag_ms));
        fetch_success = OdbcHelper::FetchResults(stmt, "ClusterTopologyQueryHelper failed to fetch topology from results");
    }
    return hosts;
}

HostInfo ClusterTopologyQueryHelper::CreateHost(SQLTCHAR* node_id, bool is_writer, SQLREAL cpu_usage, SQLREAL replica_lag_ms) {
    uint64_t weight = (std::round(replica_lag_ms) * SCALE_TO_PERCENT) + std::round(cpu_usage);
    std::string endpoint_url = GetEndpoint(node_id);
    HostInfo hi = HostInfo(endpoint_url, port, UP, is_writer, nullptr, weight);
    return hi;
}

std::string ClusterTopologyQueryHelper::GetEndpoint(SQLTCHAR* node_id) {
    std::string res(endpoint_template_);
    std::string node_id_str = StringHelper::ToString(node_id);

    int pos = res.find(REPLACE_CHAR);
    if (pos != std::string::npos) {
        res.replace(pos, 1, node_id_str);
    }
    return res;
}
