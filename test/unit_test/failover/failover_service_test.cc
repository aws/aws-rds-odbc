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

#include <gtest/gtest.h>

#include "../dialect/dialect_aurora_postgres.h"
#include "../mock_objects.h"
#include "../util/connection_string_helper.h"
#include "../util/connection_string_keys.h"

using ::testing::Return;

namespace {
    #ifdef UNICODE
    const wchar_t* conn_str = TEXT(
        "SERVER=database-pg-name.cluster-ro-XYZ.us-east-2.rds.amazonaws.com;"  \
        "ENABLECLUSTERFAILOVER=1;"                                          \
        "FAILOVERMODE=READER_OR_WRITER;"                                    \
        "READERHOSTSELECTORSTRATEGY=ROUND_ROBIN;"                           \
        "FAILOVERTIMEOUT=10000;");
    const wchar_t* mode_strict_reader = TEXT("STRICT_READER");
    const wchar_t* mode_strict_writer = TEXT("STRICT_WRITER");
    const wchar_t* mode_reader_or_writer = TEXT("READER_OR_WRITER");
    #else
    const char* conn_str = TEXT( 
        "SERVER=database-pg-name.cluster-ro-XYZ.us-east-2.rds.amazonaws.com;"  \
        "ENABLECLUSTERFAILOVER=1;"                                          \
        "FAILOVERMODE=READER_OR_WRITER;"                                    \
        "READERHOSTSELECTORSTRATEGY=ROUND_ROBIN;"                           \
        "FAILOVERTIMEOUT=10000;");
    const char* mode_strict_reader = failover_mode_str[STRICT_READER];
    const char* mode_strict_writer = failover_mode_str[STRICT_WRITER];
    const char* mode_reader_or_writer = failover_mode_str[READER_OR_WRITER];
    #endif
    const std::string server_host = "database-pg-name.cluster-ro-XYZ.us-east-2.rds.amazonaws.com";
    const std::string cluster_id = "clusterId";
    const std::shared_ptr<Dialect> driver_dialect = std::make_shared<DialectAuroraPostgres>();
    const char* failover_sql_state = "08S01"; // Communication Link Failure
    const std::string endpoint_prefix = "endpoint_url";
    const int port = 5432;
    const int host_weight = 1;
}

class FailoverServiceTest : public testing::Test {
protected:
    // Runs once per suite
    static void SetUpTestSuite() {}
    static void TearDownTestSuite() {}
    // Runs per test case
    void SetUp() override {
        topology_map = std::make_shared<SlidingCacheMap<std::string, std::vector<HostInfo>>>();
        // Mock ODBC for Cluster Monitor
        mock_odbc_helper_monitor = std::make_shared<MOCK_ODBC_HELPER>();
        EXPECT_CALL(*mock_odbc_helper_monitor, CheckResult(testing::_, testing::_, testing::_, testing::_))
            .WillRepeatedly(Return(true));
        EXPECT_CALL(*mock_odbc_helper_monitor, Cleanup(testing::_, testing::_, testing::_))
            .Times(1);
        mock_topology_monitor = std::make_shared<MOCK_TOPOLOGY_MONITOR>(mock_odbc_helper_monitor);
        EXPECT_CALL(*mock_topology_monitor, StartMonitor()).WillRepeatedly(Return());
        // Mock ODBC for Failover Service
        mock_odbc_helper = std::make_shared<MOCK_ODBC_HELPER>();

        #ifdef UNICODE
        std::map<std::wstring, std::wstring> conn_info;
        ConnectionStringHelper::ParseConnectionStringW(conn_str, conn_info);
        conn_info_ptr = std::make_shared<std::map<std::wstring, std::wstring>>(conn_info);
        #else
        std::map<std::string, std::string> conn_info;
        ConnectionStringHelper::ParseConnectionString(conn_str, conn_info);
        conn_info_ptr = std::make_shared<std::map<std::string, std::string>>(conn_info);
        #endif

        // Using HENV to dummy allocate a HDBC to set to anything other than `0` / SQL_NULL_HDBC
        OdbcHelper::AllocateHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, hdbc, "");
    }
    void TearDown() override {
        OdbcHelper::Cleanup(hdbc, SQL_NULL_HANDLE, SQL_NULL_HANDLE);
    }

    SQLHDBC hdbc = SQL_NULL_HANDLE;

    #ifdef UNICODE
    std::shared_ptr<std::map<std::wstring, std::wstring>> conn_info_ptr;
    #else
    std::shared_ptr<std::map<std::string, std::string>> conn_info_ptr;
    #endif

    std::shared_ptr<FailoverService> failover_service;
    std::vector<HostInfo> topology;
    std::shared_ptr<SlidingCacheMap<std::string, std::vector<HostInfo>>> topology_map;
    std::shared_ptr<MOCK_TOPOLOGY_MONITOR> mock_topology_monitor;
    std::shared_ptr<MOCK_ODBC_HELPER> mock_odbc_helper_monitor;
    std::shared_ptr<MOCK_ODBC_HELPER> mock_odbc_helper;
};

TEST_F(FailoverServiceTest, failover_fail_wrong_state) {
    const char* invalid_failover_sql_state = "12345";

    failover_service = std::make_shared<FailoverService>(
        server_host,
        cluster_id,
        driver_dialect,
        conn_info_ptr,
        topology_map,
        mock_topology_monitor,
        mock_odbc_helper
    );
    EXPECT_EQ(failover_service->Failover(hdbc, invalid_failover_sql_state),
        FAILOVER_SKIPPED);
}

TEST_F(FailoverServiceTest, failover_default_values_ro_cluster) {
    // Defaults to READER_OR_WRITER, ro-cluster
    conn_info_ptr->erase(FAILOVER_MODE_KEY);
    // Defaults to RANDOM_HOST_SELECTOR
    conn_info_ptr->erase(READER_HOST_SELECTOR_STRATEGY_KEY);
    // Defaults to 30ms
    conn_info_ptr->erase(FAILOVER_TIMEOUT_KEY);

    HostInfo writer_host(endpoint_prefix + "-writer", port, UP, true, nullptr, host_weight);
    HostInfo reader_host(endpoint_prefix + "-reader", port, UP, false, nullptr, host_weight);
    topology.push_back(writer_host);
    topology.push_back(reader_host);
    topology_map->Put(cluster_id, topology);

    EXPECT_CALL(*mock_topology_monitor, ForceRefresh(false, testing::_))
        .WillRepeatedly(Return(topology));
    EXPECT_CALL(*mock_odbc_helper, ConnStrConnect(testing::_, testing::_))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*mock_odbc_helper, CheckConnection(testing::_))
        .WillRepeatedly(Return(true));
    // is_connected_to_reader()
    EXPECT_CALL(*mock_odbc_helper, AllocateHandle(testing::_, testing::_, testing::_, testing::_))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*mock_odbc_helper, ExecuteQuery(testing::_, testing::_, testing::_))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*mock_odbc_helper, BindColumn(testing::_, testing::_, testing::_, testing::_, testing::_, testing::_))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*mock_odbc_helper, FetchResults(testing::_, testing::_))
        .WillRepeatedly(Return(true));

    failover_service = std::make_shared<FailoverService>(
        server_host,
        cluster_id,
        driver_dialect,
        conn_info_ptr,
        topology_map,
        mock_topology_monitor,
        mock_odbc_helper
    );
    EXPECT_EQ(failover_service->Failover(hdbc, failover_sql_state),
        FAILOVER_SUCCEED);
    // TODO - Not fully testable, can't set internal return of `is_connected_to_reader()`
    // Since this is READER_OR_WRITER, it can still pass as it is able to connect
    // Failover on READER_OR_WRITER, will try to connect to readers first
    EXPECT_EQ(failover_service->GetCurrentHost(), reader_host);
}

TEST_F(FailoverServiceTest, failover_reader_or_writer_success) {
    HostInfo writer_host(endpoint_prefix + "-writer", port, UP, true, nullptr, host_weight);
    HostInfo reader_host(endpoint_prefix + "-reader", port, UP, false, nullptr, host_weight);
    topology.push_back(writer_host);
    topology.push_back(reader_host);
    topology_map->Put(cluster_id, topology);

    EXPECT_CALL(*mock_topology_monitor, ForceRefresh(false, testing::_))
        .WillRepeatedly(Return(topology));
    EXPECT_CALL(*mock_odbc_helper, ConnStrConnect(testing::_, testing::_))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*mock_odbc_helper, CheckConnection(testing::_))
        .WillRepeatedly(Return(true));
    // is_connected_to_reader()
    EXPECT_CALL(*mock_odbc_helper, AllocateHandle(testing::_, testing::_, testing::_, testing::_))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*mock_odbc_helper, ExecuteQuery(testing::_, testing::_, testing::_))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*mock_odbc_helper, BindColumn(testing::_, testing::_, testing::_, testing::_, testing::_, testing::_))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*mock_odbc_helper, FetchResults(testing::_, testing::_))
        .WillRepeatedly(Return(true));

    failover_service = std::make_shared<FailoverService>(
        server_host,
        cluster_id,
        driver_dialect,
        conn_info_ptr,
        topology_map,
        mock_topology_monitor,
        mock_odbc_helper
    );
    EXPECT_EQ(failover_service->Failover(hdbc, failover_sql_state),
        FAILOVER_SUCCEED);
    // TODO - Not fully testable, can't set internal return of `is_connected_to_reader()`
    // Since this is READER_OR_WRITER, it can still pass as it is able to connect
    // Failover on READER_OR_WRITER, will try to connect to readers first
    EXPECT_EQ(failover_service->GetCurrentHost(), reader_host);
}

// TODO - Not testable, can't set internal return of `is_connected_to_reader()` for strict reader
TEST_F(FailoverServiceTest, DISABLED_failover_strict_reader_success) {
    conn_info_ptr->insert_or_assign(FAILOVER_MODE_KEY, mode_strict_reader);

    HostInfo writer_host(endpoint_prefix + "-writer", port, UP, true, nullptr, host_weight);
    HostInfo reader_host(endpoint_prefix + "-reader", port, UP, false, nullptr, host_weight);
    topology.push_back(writer_host);
    topology.push_back(reader_host);
    topology_map->Put(cluster_id, topology);

    EXPECT_CALL(*mock_topology_monitor, ForceRefresh(false, testing::_))
        .WillRepeatedly(Return(topology));
    EXPECT_CALL(*mock_odbc_helper, ConnStrConnect(testing::_, testing::_))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*mock_odbc_helper, CheckConnection(testing::_))
        .WillRepeatedly(Return(true));
    // is_connected_to_reader()
    EXPECT_CALL(*mock_odbc_helper, AllocateHandle(testing::_, testing::_, testing::_, testing::_))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*mock_odbc_helper, ExecuteQuery(testing::_, testing::_, testing::_))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*mock_odbc_helper, BindColumn(testing::_, testing::_, testing::_, testing::_, testing::_, testing::_))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*mock_odbc_helper, FetchResults(testing::_, testing::_))
        .WillRepeatedly(Return(true));

    failover_service = std::make_shared<FailoverService>(
        server_host,
        cluster_id,
        driver_dialect,
        conn_info_ptr,
        topology_map,
        mock_topology_monitor,
        mock_odbc_helper
    );
    EXPECT_TRUE(failover_service->Failover(hdbc, failover_sql_state));
    // EXPECT_EQ(failover_service->GetCurrentHost(), reader_host);
}

TEST_F(FailoverServiceTest, failover_strict_writer_success) {
    conn_info_ptr->insert_or_assign(FAILOVER_MODE_KEY, mode_strict_writer);

    HostInfo writer_host(endpoint_prefix + "-writer", port, UP, true, nullptr, host_weight);
    HostInfo reader_host(endpoint_prefix + "-reader", port, UP, false, nullptr, host_weight);
    topology.push_back(writer_host);
    topology.push_back(reader_host);
    topology_map->Put(cluster_id, topology);

    EXPECT_CALL(*mock_topology_monitor, ForceRefresh(true, testing::_))
        .WillRepeatedly(Return(topology));
    EXPECT_CALL(*mock_odbc_helper, ConnStrConnect(testing::_, testing::_))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*mock_odbc_helper, CheckConnection(testing::_))
        .WillRepeatedly(Return(true));
    // is_connected_to_reader()
    EXPECT_CALL(*mock_odbc_helper, AllocateHandle(testing::_, testing::_, testing::_, testing::_))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*mock_odbc_helper, ExecuteQuery(testing::_, testing::_, testing::_))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*mock_odbc_helper, BindColumn(testing::_, testing::_, testing::_, testing::_, testing::_, testing::_))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*mock_odbc_helper, FetchResults(testing::_, testing::_))
        .WillRepeatedly(Return(true));

    failover_service = std::make_shared<FailoverService>(
        server_host,
        cluster_id,
        driver_dialect,
        conn_info_ptr,
        topology_map,
        mock_topology_monitor,
        mock_odbc_helper
    );
    EXPECT_TRUE(failover_service->Failover(hdbc, failover_sql_state));
    // TODO - Not fully testable, can't set internal return of `is_connected_to_reader()`
    // This still works due to how the default will say it is NOT connected to a reader
    EXPECT_EQ(failover_service->GetCurrentHost(), writer_host);
}

TEST_F(FailoverServiceTest, failover_fail_no_hosts) {
    topology.clear();
    topology_map->Put(cluster_id, topology);

    EXPECT_CALL(*mock_topology_monitor, ForceRefresh(false, testing::_))
        .WillRepeatedly(Return(topology));

    failover_service = std::make_shared<FailoverService>(
        server_host,
        cluster_id,
        driver_dialect,
        conn_info_ptr,
        topology_map,
        mock_topology_monitor,
        mock_odbc_helper
    );

    EXPECT_EQ(failover_service->Failover(hdbc, failover_sql_state),
        FAILOVER_FAILED);
}

TEST_F(FailoverServiceTest, failover_strict_reader_fail_no_reader) {
    conn_info_ptr->insert_or_assign(FAILOVER_MODE_KEY, mode_strict_reader);
    HostInfo writer_host(endpoint_prefix + "-writer", port, UP, true, nullptr, host_weight);
    topology.push_back(writer_host);
    topology_map->Put(cluster_id, topology);

    EXPECT_CALL(*mock_topology_monitor, ForceRefresh(false, testing::_))
        .WillRepeatedly(Return(topology));
    EXPECT_CALL(*mock_odbc_helper, ConnStrConnect(testing::_, testing::_))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*mock_odbc_helper, CheckConnection(testing::_))
        .WillRepeatedly(Return(true));
    // is_connected_to_reader()
    EXPECT_CALL(*mock_odbc_helper, AllocateHandle(testing::_, testing::_, testing::_, testing::_))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*mock_odbc_helper, ExecuteQuery(testing::_, testing::_, testing::_))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*mock_odbc_helper, BindColumn(testing::_, testing::_, testing::_, testing::_, testing::_, testing::_))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*mock_odbc_helper, FetchResults(testing::_, testing::_))
        .WillRepeatedly(Return(true));

    failover_service = std::make_shared<FailoverService>(
        server_host,
        cluster_id,
        driver_dialect,
        conn_info_ptr,
        topology_map,
        mock_topology_monitor,
        mock_odbc_helper
    );

    EXPECT_EQ(failover_service->Failover(hdbc, failover_sql_state),
        FAILOVER_FAILED);
}

TEST_F(FailoverServiceTest, failover_strict_writer_fail_no_writer) {
    conn_info_ptr->insert_or_assign(FAILOVER_MODE_KEY, mode_strict_writer);
    HostInfo reader_host(endpoint_prefix + "-reader", port, UP, false, nullptr, host_weight);
    topology.push_back(reader_host);
    topology_map->Put(cluster_id, topology);

    EXPECT_CALL(*mock_topology_monitor, ForceRefresh(true, testing::_))
        .WillRepeatedly(Return(topology));

    failover_service = std::make_shared<FailoverService>(
        server_host,
        cluster_id,
        driver_dialect,
        conn_info_ptr,
        topology_map,
        mock_topology_monitor,
        mock_odbc_helper
    );

    EXPECT_EQ(failover_service->Failover(hdbc, failover_sql_state),
        FAILOVER_FAILED);
}
