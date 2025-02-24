//  Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Library General Public
//  License as published by the Free Software Foundation; either
//  version 2 of the License, or (at your option) any later version.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Library General Public License for more details.
//
//  You should have received a copy of the GNU Library General Public
//  License along with this library; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

#ifndef CLUSTER_TOPOLOGY_QUERY_HELPER_H
#define CLUSTER_TOPOLOGY_QUERY_HELPER_H

#include <vector>

#ifdef WIN32
    #include <windows.h>
#endif
#include <sql.h>
#include <sqlext.h>
#include <sqltypes.h>

#include "../host_info.h"
#include "../util/logger_wrapper.h"
#include "../util/odbc_helper.h"

class ClusterTopologyQueryHelper {    
public:
    ClusterTopologyQueryHelper(int port, const std::string& endpoint_template, const std::string& topology_query, const std::string& writer_id_query, const std::string& node_id_query);
    virtual std::string get_writer_id(SQLHDBC hdbc);
    virtual std::string get_node_id(SQLHDBC hdbc);
    virtual std::vector<HostInfo> query_topology(SQLHDBC hdbc);
    virtual HostInfo create_host(SQLTCHAR* node_id, bool is_writer, SQLREAL cpu_usage, SQLREAL replica_lag_ms, SQL_TIMESTAMP_STRUCT update_timestamp);
    virtual std::string get_endpoint(SQLTCHAR* node_id);

private:
    const int port;

    // Query & Template to be passed in from caller, below are examples of APG
    // ?.cluster-<Cluster-ID>.<Region>.rds.amazonaws.com
#ifdef UNICODE
    std::wstring endpoint_template_;
    // SELECT SERVER_ID, CASE WHEN SESSION_ID = 'MASTER_SESSION_ID' THEN TRUE ELSE FALSE END, CPU, COALESCE(REPLICA_LAG_IN_MSEC, 0), LAST_UPDATE_TIMESTAMP FROM aurora_replica_status() WHERE EXTRACT(EPOCH FROM(NOW() - LAST_UPDATE_TIMESTAMP)) <= 300 OR SESSION_ID = 'MASTER_SESSION_ID' OR LAST_UPDATE_TIMESTAMP IS NULL
    std::wstring topology_query_;
    // SELECT SERVER_ID FROM aurora_replica_status() WHERE SESSION_ID = 'MASTER_SESSION_ID' AND SERVER_ID = aurora_db_instance_identifier()
    std::wstring writer_id_query_;
    // SELECT aurora_db_instance_identifier()
    std::wstring node_id_query_;
#else
    std::string endpoint_template_;
    std::string topology_query_;
    std::string writer_id_query_;
    std::string node_id_query_;
#endif

#ifdef UNICODE
    static constexpr wchar_t REPLACE_CHAR = L'?';
#else 
    static constexpr char REPLACE_CHAR = '?';
#endif

    static constexpr int BUFFER_SIZE = 1024;;
    static constexpr uint64_t SCALE_TO_PERCENT = 100L;

    // Topology Query
    static constexpr int NODE_ID_COL = 1;
    static constexpr int IS_WRITER_COL = 2;
    static constexpr int CPU_USAGE_COL = 3;
    static constexpr int REPLICA_LAG_COL = 4;
    static constexpr int UPDATE_TIMESTAMP_COL = 5;
};

#endif // CLUSTER_TOPOLOGY_QUERY_HELPER_H
