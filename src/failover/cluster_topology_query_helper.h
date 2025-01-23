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

#define AS_SQLCHAR(str) (const_cast<SQLTCHAR*>(reinterpret_cast<const SQLTCHAR*>(str)))
#define AS_CHAR(str) (reinterpret_cast<char*>(str))

class ClusterTopologyQueryHelper {    
public:
    ClusterTopologyQueryHelper(int port, const std::string& endpoint_template, const std::string& topology_query, const std::string& writer_id_query, const std::string& node_id_query);
    virtual std::string get_writer_id(SQLHDBC hdbc);
    virtual std::string get_node_id(SQLHDBC hdbc);
    virtual std::vector<HostInfo> query_topology(SQLHDBC hdbc);
    virtual HostInfo create_host(SQLTCHAR* node_id, bool is_writer, SQLFLOAT cpu_usage, SQLFLOAT replica_lag_ms, SQL_TIMESTAMP_STRUCT update_timestamp);
    virtual std::string get_endpoint(SQLTCHAR* node_id);

private:
    int port;

    // Query & Template to be passed in from caller, below are examples of APG
    // ?.cluster-<Cluster-ID>.<Region>.rds.amazonaws.com
    std::string endpoint_template;
    // SELECT SERVER_ID, CASE WHEN SESSION_ID = 'MASTER_SESSION_ID' THEN TRUE ELSE FALSE END, CPU, COALESCE(REPLICA_LAG_IN_MSEC, 0), LAST_UPDATE_TIMESTAMP FROM aurora_replica_status() WHERE EXTRACT(EPOCH FROM(NOW() - LAST_UPDATE_TIMESTAMP)) <= 300 OR SESSION_ID = 'MASTER_SESSION_ID' OR LAST_UPDATE_TIMESTAMP IS NULL
    std::string topology_query;
    // SELECT SERVER_ID FROM aurora_replica_status() WHERE SESSION_ID = 'MASTER_SESSION_ID' AND SERVER_ID = aurora_db_instance_identifier()
    std::string writer_id_query;
    // SELECT aurora_db_instance_identifier()
    std::string node_id_query;

    const static int BUFFER_SIZE = 1024;
    const static char REPLACE_CHAR = '?';
    const static uint64_t SCALE_TO_PERCENT = 100L;

    // Topology Query
    const static int NODE_ID_COL = 1;
    const static int IS_WRITER_COL = 2;
    const static int CPU_USAGE_COL = 3;
    const static int REPLICA_LAG_COL = 4;
    const static int UPDATE_TIMESTAMP_COL = 5;
};

#endif // CLUSTER_TOPOLOGY_QUERY_HELPER_H
