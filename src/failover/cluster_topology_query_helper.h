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
    virtual std::string GetWriterId(SQLHDBC hdbc);
    virtual std::string GetNodeId(SQLHDBC hdbc);
    virtual std::vector<HostInfo> QueryTopology(SQLHDBC hdbc);
    virtual HostInfo CreateHost(SQLTCHAR* node_id, bool is_writer, SQLREAL cpu_usage, SQLREAL replica_lag_ms);
    virtual std::string GetEndpoint(SQLTCHAR* node_id);

private:
    const int port;

    // Query & Template to be passed in from caller, below are examples of APG
    // ?.cluster-<Cluster-ID>.<Region>.rds.amazonaws.com
#ifdef UNICODE
    std::wstring endpoint_template_;
    // SELECT SERVER_ID, CASE WHEN SESSION_ID = 'MASTER_SESSION_ID' THEN TRUE ELSE FALSE END, CPU, COALESCE(REPLICA_LAG_IN_MSEC, 0) FROM aurora_replica_status() WHERE EXTRACT(EPOCH FROM(NOW() - LAST_UPDATE_TIMESTAMP)) <= 300 OR SESSION_ID = 'MASTER_SESSION_ID' OR LAST_UPDATE_TIMESTAMP IS NULL
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
};

#endif // CLUSTER_TOPOLOGY_QUERY_HELPER_H
