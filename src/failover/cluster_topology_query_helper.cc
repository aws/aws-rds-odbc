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

#include <cmath>

ClusterTopologyQueryHelper::ClusterTopologyQueryHelper(
    int port,
    const std::string& endpoint_template,
    const std::string& topology_query,
    const std::string& writer_id_query,
    const std::string& node_id_query):
    port{port} {
    #ifdef UNICODE
    endpoint_template_ = StringHelper::ToWstring(endpoint_template);
    topology_query_ = StringHelper::ToWstring(topology_query);
    writer_id_query_ = StringHelper::ToWstring(writer_id_query);
    node_id_query_ = StringHelper::ToWstring(node_id_query);
    #else
        endpoint_template_ = endpoint_template;
        topology_query_ = topology_query;
        writer_id_query_ = writer_id_query;
        node_id_query_ = node_id_query;
    #endif
}

std::string ClusterTopologyQueryHelper::GetWriterId(SQLHDBC hdbc) {
    SQLRETURN rc;
    SQLHSTMT stmt = SQL_NULL_HANDLE;
    SQLTCHAR writer_id[BUFFER_SIZE] = {0};

    if (!OdbcHelper::AllocateHandle(SQL_HANDLE_STMT, hdbc, stmt, "ClusterTopologyQueryHelper failed to allocate handle")) {
        return std::string();
    }

    if (!OdbcHelper::ExecuteQuery(stmt, AS_SQLTCHAR(writer_id_query_.c_str()), "ClusterTopologyQueryHelper failed to execute writer query")) {
        return std::string();
    }

    if (!OdbcHelper::BindColumn(stmt, NODE_ID_COL, SQL_C_TCHAR, writer_id, sizeof(writer_id), "ClusterTopologyQueryHelper failed to bind writer_id column")) {
        return std::string();
    }

    if (!OdbcHelper::FetchResults(stmt, "ClusterTopologyQueryHelper failed to fetch writer from results")) {
        return std::string();
    }

    OdbcHelper::Cleanup(SQL_NULL_HANDLE, SQL_NULL_HANDLE, stmt);
#ifdef UNICODE
    return StringHelper::ToString(AS_WCHAR(writer_id));
#endif
    return std::string(AS_CHAR(writer_id));
}

std::string ClusterTopologyQueryHelper::GetNodeId(SQLHDBC hdbc) {
    SQLHSTMT stmt = SQL_NULL_HANDLE;
    SQLTCHAR node_id[BUFFER_SIZE] = {0};

    if (!OdbcHelper::AllocateHandle(SQL_HANDLE_STMT, hdbc, stmt, "ClusterTopologyQueryHelper failed to allocate handle")) {
        return std::string();
    }

    if (!OdbcHelper::ExecuteQuery(stmt, AS_SQLTCHAR(node_id_query_.c_str()), "ClusterTopologyQueryHelper failed to execute node ID query")) {
        return std::string();
    }

    if (!OdbcHelper::BindColumn(stmt, NODE_ID_COL, SQL_C_TCHAR, node_id, sizeof(node_id), "ClusterTopologyQueryHelper failed to bind node_id column")) {
        return std::string();
    }

    if (!OdbcHelper::FetchResults(stmt, "ClusterTopologyQueryHelper failed to fetch ndoe ID from results")) {
        return std::string();
    }

    OdbcHelper::Cleanup(SQL_NULL_HANDLE, SQL_NULL_HANDLE, stmt);
#ifdef UNICODE
    return StringHelper::ToString(AS_WCHAR(node_id));
#endif
    return std::string(AS_CHAR(node_id));
}

std::vector<HostInfo> ClusterTopologyQueryHelper::QueryTopology(SQLHDBC hdbc) {
    SQLHSTMT stmt = SQL_NULL_HANDLE;
    SQLTCHAR node_id[BUFFER_SIZE] = {0};
    bool is_writer;
    SQLREAL cpu_usage;
    SQLINTEGER replica_lag_ms;

    if (!OdbcHelper::AllocateHandle(SQL_HANDLE_STMT, hdbc, stmt, "ClusterTopologyQueryHelper failed to allocate handle")) {
        return std::vector<HostInfo>();
    }

    if (!OdbcHelper::ExecuteQuery(stmt, AS_SQLTCHAR(topology_query_.c_str()), "ClusterTopologyQueryHelper failed to execute topology query")) {
        return std::vector<HostInfo>();
    }

    if (!OdbcHelper::BindColumn(stmt, NODE_ID_COL, SQL_C_TCHAR, node_id, sizeof(node_id), "ClusterTopologyQueryHelper failed to bind node_id column")) {
        return std::vector<HostInfo>();
    }
    if (!OdbcHelper::BindColumn(stmt, IS_WRITER_COL, SQL_BIT, &is_writer, sizeof(is_writer), "ClusterTopologyQueryHelper failed to bind is_writer column")) {
        return std::vector<HostInfo>();
    }
    if (!OdbcHelper::BindColumn(stmt, CPU_USAGE_COL, SQL_REAL, &cpu_usage, sizeof(cpu_usage), "ClusterTopologyQueryHelper failed to bind cpu_usage column")) {
        return std::vector<HostInfo>();
    }
    if (!OdbcHelper::BindColumn(stmt, REPLICA_LAG_COL, SQL_INTEGER, &replica_lag_ms, sizeof(replica_lag_ms), "ClusterTopologyQueryHelper failed to bind replica_lag_ms column")) {
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
#ifdef UNICODE
    std::string res = StringHelper::ToString(endpoint_template_);
    std::string node_id_str = StringHelper::ToString(AS_WCHAR(node_id));
#else
    std::string res(endpoint_template_);
    std::string node_id_str = AS_CHAR(node_id);
#endif
    int pos = res.find(REPLACE_CHAR);
    if (pos != std::string::npos) {
        res.replace(pos, 1, node_id_str);
    }
    return res;
}
