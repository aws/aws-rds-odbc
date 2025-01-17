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

ClusterTopologyQueryHelper::ClusterTopologyQueryHelper(
    const int port,
    const std::string& endpoint_template,
    const std::string& topology_query,
    const std::string& writer_topology_query,
    const std::string& node_id_query):
    port{port},
    endpoint_template{std::move(endpoint_template)},
    topology_query{std::move(topology_query)},
    writer_id_query{std::move(writer_id_query)},
    node_id_query{std::move(node_id_query)} {}

std::string ClusterTopologyQueryHelper::get_writer_id(SQLHDBC hdbc) {
    SQLRETURN rc;
    SQLHSTMT stmt = SQL_NULL_HANDLE;
    SQLTCHAR writer_id[BUFFER_SIZE];
    SQLLEN out_length = 0;

    rc = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &stmt);
    if (!OdbcHelper::CheckResult(rc, std::string("ClusterTopologyQueryHelper failed to allocate handle"), stmt, SQL_HANDLE_STMT)) {
        OdbcHelper::Cleanup(SQL_NULL_HANDLE, SQL_NULL_HANDLE, stmt);
        return std::string();
    }

    rc = SQLExecDirect(stmt, AS_SQLCHAR(writer_id_query.c_str()), SQL_NTS);    
    if (!OdbcHelper::CheckResult(rc, std::string("ClusterTopologyQueryHelper failed to execute writer query"), stmt, SQL_HANDLE_STMT)) {
        OdbcHelper::Cleanup(SQL_NULL_HANDLE, SQL_NULL_HANDLE, stmt);
        return std::string();
    }

    rc = SQLBindCol(stmt, 1, SQL_C_CHAR, writer_id, sizeof(writer_id), &out_length);
    if (!OdbcHelper::CheckResult(rc, std::string("ClusterTopologyQueryHelper failed to bind writer_id column"), stmt, SQL_HANDLE_STMT)) {
        OdbcHelper::Cleanup(SQL_NULL_HANDLE, SQL_NULL_HANDLE, stmt);
        return std::string();
    }

    rc = SQLFetch(stmt);
    if (!OdbcHelper::CheckResult(rc, std::string("ClusterTopologyQueryHelper failed to fetch writer from results"), stmt, SQL_HANDLE_STMT)) {
        OdbcHelper::Cleanup(SQL_NULL_HANDLE, SQL_NULL_HANDLE, stmt);
        return std::string();
    }

    OdbcHelper::Cleanup(SQL_NULL_HANDLE, SQL_NULL_HANDLE, stmt);
    return std::string(AS_CHAR(writer_id), out_length);    
}

std::vector<HostInfo> ClusterTopologyQueryHelper::query_topology(SQLHDBC hdbc) {
    SQLRETURN rc;
    SQLHSTMT stmt = SQL_NULL_HANDLE;
    SQLTCHAR node_id[BUFFER_SIZE];
    SQLLEN out_length = 0;
    SQLINTEGER is_writer;
    SQLFLOAT cpu_usage;
    SQLFLOAT replica_lag_ms;
    SQL_TIMESTAMP_STRUCT last_update_timestamp;

    rc = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &stmt);
    if (!OdbcHelper::CheckResult(rc, std::string("ClusterTopologyQueryHelper failed to allocate handle"), stmt, SQL_HANDLE_STMT)) {
        OdbcHelper::Cleanup(SQL_NULL_HANDLE, SQL_NULL_HANDLE, stmt);
        return std::vector<HostInfo>();
    }

    rc = SQLExecDirect(stmt, AS_SQLCHAR(topology_query.c_str()), SQL_NTS);    
    if (!OdbcHelper::CheckResult(rc, std::string("ClusterTopologyQueryHelper failed to execute topology query"), stmt, SQL_HANDLE_STMT)) {
        OdbcHelper::Cleanup(SQL_NULL_HANDLE, SQL_NULL_HANDLE, stmt);
        return std::vector<HostInfo>();
    }

    rc = SQLBindCol(stmt, 1, SQL_C_CHAR, node_id, sizeof(node_id), &out_length);
    if (!OdbcHelper::CheckResult(rc, std::string("ClusterTopologyQueryHelper failed to bind node_id column"), stmt, SQL_HANDLE_STMT)) {
        OdbcHelper::Cleanup(SQL_NULL_HANDLE, SQL_NULL_HANDLE, stmt);
        return std::vector<HostInfo>();
    }
    rc = SQLBindCol(stmt, 2, SQL_C_SLONG, &is_writer, sizeof(is_writer), nullptr);
    if (!OdbcHelper::CheckResult(rc, std::string("ClusterTopologyQueryHelper failed to bind is_writer column"), stmt, SQL_HANDLE_STMT)) {
        OdbcHelper::Cleanup(SQL_NULL_HANDLE, SQL_NULL_HANDLE, stmt);
        return std::vector<HostInfo>();
    }
    rc = SQLBindCol(stmt, 3, SQL_INTEGER, &cpu_usage, sizeof(cpu_usage), nullptr);
    if (!OdbcHelper::CheckResult(rc, std::string("ClusterTopologyQueryHelper failed to bind cpu_usage column"), stmt, SQL_HANDLE_STMT)) {
        OdbcHelper::Cleanup(SQL_NULL_HANDLE, SQL_NULL_HANDLE, stmt);
        return std::vector<HostInfo>();
    }
    rc = SQLBindCol(stmt, 4, SQL_C_SLONG, &replica_lag_ms, sizeof(replica_lag_ms), nullptr);
    if (!OdbcHelper::CheckResult(rc, std::string("ClusterTopologyQueryHelper failed to bind replica_lag_ms column"), stmt, SQL_HANDLE_STMT)) {
        OdbcHelper::Cleanup(SQL_NULL_HANDLE, SQL_NULL_HANDLE, stmt);
        return std::vector<HostInfo>();
    }
    rc = SQLBindCol(stmt, 5, SQL_C_TIMESTAMP, &last_update_timestamp, sizeof(last_update_timestamp), nullptr);
    if (!OdbcHelper::CheckResult(rc, std::string("ClusterTopologyQueryHelper failed to bind last_update_timestamp column"), stmt, SQL_HANDLE_STMT)) {
        OdbcHelper::Cleanup(SQL_NULL_HANDLE, SQL_NULL_HANDLE, stmt);
        return std::vector<HostInfo>();
    }

    std::vector<HostInfo> hosts;
    while (OdbcHelper::CheckResult(SQLFetch(stmt), std::string("ClusterTopologyQueryHelper failed to fetch topology from results"), stmt, SQL_HANDLE_STMT)) {
        hosts.push_back(create_host(node_id, is_writer, cpu_usage, replica_lag_ms, last_update_timestamp));
    }

    OdbcHelper::Cleanup(SQL_NULL_HANDLE, SQL_NULL_HANDLE, stmt);
    return hosts;
}

HostInfo ClusterTopologyQueryHelper::create_host(SQLTCHAR* node_id, bool is_writer, SQLFLOAT cpu_usage, SQLFLOAT replica_lag_ms, SQL_TIMESTAMP_STRUCT update_timestamp) {
    long weight = std::round(replica_lag_ms) * 100L + std::round(cpu_usage);
    std::string endpoint_url = get_endpoint(node_id);
    HostInfo hi = HostInfo(endpoint_url, port, HOST_STATE::UP, true, nullptr, weight, update_timestamp);
    return hi;
}

std::string ClusterTopologyQueryHelper::get_endpoint(SQLTCHAR* node_id) {
    std::string res(endpoint_template); // Create a copy
    int pos = res.find(REPLACE_CHAR);
    if (pos != std::string::npos) {
        res.replace(pos, 1, AS_CHAR(node_id));
    }
    return res;
}
