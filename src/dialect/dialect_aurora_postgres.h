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

#ifndef DIALECT_AURORA_POSTGRES_H
#define DIALECT_AURORA_POSTGRES_H

#include "dialect.h"

class DialectAuroraPostgres : public Dialect {
public:
    int GetDefaultPort() override { return DEFAULT_POSTGRES_PORT; };
    std::string GetTopologyQuery() override { return TOPOLOGY_QUERY; };
    std::string GetWriterIdQuery() override { return WRITER_ID_QUERY; };
    std::string GetNodeIdQuery() override { return NODE_ID_QUERY; };
    std::string GetIsReaderQuery() override { return IS_READER_QUERY; };

private:
    const int DEFAULT_POSTGRES_PORT = 5432;

    const std::string TOPOLOGY_QUERY =
        "SELECT SERVER_ID, CASE WHEN SESSION_ID = 'MASTER_SESSION_ID' THEN TRUE ELSE FALSE END, \
            CPU, COALESCE(REPLICA_LAG_IN_MSEC, 0) \
            FROM aurora_replica_status() \
            WHERE EXTRACT(EPOCH FROM(NOW() - LAST_UPDATE_TIMESTAMP)) <= 300 OR SESSION_ID = 'MASTER_SESSION_ID' \
            OR LAST_UPDATE_TIMESTAMP IS NULL";

    const std::string WRITER_ID_QUERY =
        "SELECT SERVER_ID FROM aurora_replica_status() \
        WHERE SESSION_ID = 'MASTER_SESSION_ID' \
        AND SERVER_ID = aurora_db_instance_identifier()";

    const std::string NODE_ID_QUERY =
        "SELECT aurora_db_instance_identifier()";

    const std::string IS_READER_QUERY =
        "SELECT pg_is_in_recovery()";
};

#endif // DIALECT_AURORA_POSTGRES_H
