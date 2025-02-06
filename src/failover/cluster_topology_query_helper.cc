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
        endpoint_template_ = std::wstring(endpoint_template.begin(), endpoint_template.end());
        topology_query_ = std::wstring(topology_query.begin(), topology_query.end());
        writer_id_query_ = std::wstring(writer_id_query.begin(), writer_id_query.end());
        node_id_query_ = std::wstring(node_id_query.begin(), node_id_query.end());
    #else
        endpoint_template_ = endpoint_template;
        topology_query_ = topology_query;
        writer_id_query_ = writer_id_query;
        node_id_query_ = node_id_query;
    #endif
    }

std::string ClusterTopologyQueryHelper::get_writer_id(SQLHDBC hdbc) {
    SQLRETURN rc;
    SQLHSTMT stmt = SQL_NULL_HANDLE;
    SQLTCHAR writer_id[BUFFER_SIZE];

    if (!OdbcHelper::AllocateHandle(SQL_HANDLE_STMT, hdbc, stmt, "ClusterTopologyQueryHelper failed to allocate handle")) {
        return std::string();
    }

    if (!OdbcHelper::ExecuteQuery(stmt, AS_SQLTCHAR(writer_id_query_.c_str()), "ClusterTopologyQueryHelper failed to execute writer query")) {
        return std::string();
    }

    if (!OdbcHelper::BindColumn(stmt, NODE_ID_COL, SQL_C_CHAR, writer_id, sizeof(writer_id), "ClusterTopologyQueryHelper failed to bind writer_id column")) {
        return std::string();
    }

    if (!OdbcHelper::FetchResults(stmt, "ClusterTopologyQueryHelper failed to fetch writer from results")) {
        return std::string();
    }

    OdbcHelper::Cleanup(SQL_NULL_HANDLE, SQL_NULL_HANDLE, stmt);
    return std::string(AS_CHAR(writer_id));
}

std::string ClusterTopologyQueryHelper::get_node_id(SQLHDBC hdbc) {
    SQLRETURN rc;
    SQLHSTMT stmt = SQL_NULL_HANDLE;
    SQLTCHAR writer_id[BUFFER_SIZE];

    if (!OdbcHelper::AllocateHandle(SQL_HANDLE_STMT, hdbc, stmt, "ClusterTopologyQueryHelper failed to allocate handle")) {
        return std::string();
    }

    if (!OdbcHelper::ExecuteQuery(stmt, AS_SQLTCHAR(node_id_query_.c_str()), "ClusterTopologyQueryHelper failed to execute writer query")) {
        return std::string();
    }

    if (!OdbcHelper::BindColumn(stmt, NODE_ID_COL, SQL_C_CHAR, writer_id, sizeof(writer_id), "ClusterTopologyQueryHelper failed to bind writer_id column")) {
        return std::string();
    }

    if (!OdbcHelper::FetchResults(stmt, "ClusterTopologyQueryHelper failed to fetch writer from results")) {
        return std::string();
    }

    OdbcHelper::Cleanup(SQL_NULL_HANDLE, SQL_NULL_HANDLE, stmt);
    return std::string(AS_CHAR(writer_id));
}

std::vector<HostInfo> ClusterTopologyQueryHelper::query_topology(SQLHDBC hdbc) {
    SQLHSTMT stmt = SQL_NULL_HANDLE;
    SQLTCHAR node_id[BUFFER_SIZE];
    bool is_writer;
    SQLREAL cpu_usage;
    SQLREAL replica_lag_ms;
    SQL_TIMESTAMP_STRUCT last_update_timestamp{};

    if (!OdbcHelper::AllocateHandle(SQL_HANDLE_STMT, hdbc, stmt, "ClusterTopologyQueryHelper failed to allocate handle")) {
        return std::vector<HostInfo>();
    }

    if (!OdbcHelper::ExecuteQuery(stmt, AS_SQLTCHAR(topology_query_.c_str()), "ClusterTopologyQueryHelper failed to execute topology query")) {
        return std::vector<HostInfo>();
    }

    if (!OdbcHelper::BindColumn(stmt, NODE_ID_COL, SQL_CHAR, node_id, sizeof(node_id), "ClusterTopologyQueryHelper failed to bind node_id column")) {
        return std::vector<HostInfo>();
    }
    if (!OdbcHelper::BindColumn(stmt, IS_WRITER_COL, SQL_BIT, &is_writer, sizeof(is_writer), "ClusterTopologyQueryHelper failed to bind is_writer column")) {
        return std::vector<HostInfo>();
    }
    if (!OdbcHelper::BindColumn(stmt, CPU_USAGE_COL, SQL_REAL, &cpu_usage, sizeof(cpu_usage), "ClusterTopologyQueryHelper failed to bind cpu_usage column")) {
        return std::vector<HostInfo>();
    }
    if (!OdbcHelper::BindColumn(stmt, REPLICA_LAG_COL, SQL_REAL, &replica_lag_ms, sizeof(replica_lag_ms), "ClusterTopologyQueryHelper failed to bind replica_lag_ms column")) {
        return std::vector<HostInfo>();
    }
    // TODO(yuenhcol), disabled for now, causes issues and is not useful at the time of writing
    // if (!OdbcHelper::BindColumn(stmt, UPDATE_TIMESTAMP_COL, SQL_C_TYPE_TIMESTAMP, &last_update_timestamp, sizeof(last_update_timestamp), "ClusterTopologyQueryHelper failed to bind last_update_timestamp column")) {
    //     return std::vector<HostInfo>();
    // }

    bool fetch_success = OdbcHelper::FetchResults(stmt, "ClusterTopologyQueryHelper failed to fetch topology from results");
    std::vector<HostInfo> hosts;
    while (fetch_success) {
        hosts.push_back(create_host(node_id, is_writer, cpu_usage, replica_lag_ms, last_update_timestamp));
        fetch_success = OdbcHelper::FetchResults(stmt, "ClusterTopologyQueryHelper failed to fetch topology from results");
    }

    if (fetch_success) {
        OdbcHelper::Cleanup(SQL_NULL_HANDLE, SQL_NULL_HANDLE, stmt);
    }
    return hosts;
}

HostInfo ClusterTopologyQueryHelper::create_host(SQLTCHAR* node_id, bool is_writer, SQLREAL cpu_usage, SQLREAL replica_lag_ms, SQL_TIMESTAMP_STRUCT update_timestamp) {
    uint64_t weight = (std::round(replica_lag_ms) * SCALE_TO_PERCENT) + std::round(cpu_usage);
    std::string endpoint_url = get_endpoint(node_id);
    HostInfo hi = HostInfo(endpoint_url, port, HOST_STATE::UP, is_writer, nullptr, weight, update_timestamp);
    return hi;
}

#ifdef UNICODE
std::string ClusterTopologyQueryHelper::get_endpoint(SQLTCHAR* node_id) {
    std::wstring w_res(endpoint_template_); // Create a copy
    int pos = w_res.find(REPLACE_CHAR);
    if (pos != std::wstring::npos) {
        w_res.replace(pos, 1, AS_WCHAR(node_id));
    }
    std::string res(w_res.length(), 0);
    // Simple transform is fine for node IDs
    // Instance names are restricted to alphanumberic characters
    std::transform(w_res.begin(), w_res.end(), res.begin(), [] (wchar_t c) {
        return static_cast<char>(c);
    });
    return res;
}
#else
std::string ClusterTopologyQueryHelper::get_endpoint(SQLTCHAR* node_id) {
    std::string res(endpoint_template_); // Create a copy
    int pos = res.find(REPLACE_CHAR);
    if (pos != std::string::npos) {
        res.replace(pos, 1, AS_CHAR(node_id));
    }
    return res;
}
#endif
