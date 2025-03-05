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

#include <gtest/gtest.h>
#include <cluster_topology_monitor.h>

#include "../mock_objects.h"
#include "text_helper.h"

using ::testing::Return;

namespace {
  const std::string cluster_id = "cluster_test_id";
  const SQLTCHAR* conn_str = (SQLTCHAR *)TEXT("DSN=Test_DSN;Server=cluster.server.com;");
  const long ignore_topology_request_ns = 1000;
  const long high_refresh_rate_ns = 50;
  const long refresh_rate_ns = 100;
  const long sleep_duration_sec = 5;
}

class ClusterTopologyMonitorTest : public testing::Test {
  protected:
    // Runs once per suite
    static void SetUpTestSuite() {}
    static void TearDownTestSuite() {}
    // Runs per test case
    void SetUp() override {
      topology_map = std::make_shared<SlidingCacheMap<std::string, std::vector<HostInfo>>>();
      mock_odbc_helper = std::make_shared<MOCK_ODBC_HELPER>();
      mock_query_helper = std::make_shared<MOCK_CLUSTER_TOPOLOGY_QUERY_HELPER>();
    }
    void TearDown() override {}

    std::shared_ptr<ClusterTopologyMonitor> monitor;

    std::shared_ptr<MOCK_ODBC_HELPER> mock_odbc_helper;
    std::shared_ptr<MOCK_CLUSTER_TOPOLOGY_QUERY_HELPER> mock_query_helper;
    std::shared_ptr<SlidingCacheMap<std::string, std::vector<HostInfo>>> topology_map;
};

TEST_F(ClusterTopologyMonitorTest, henv_create_fail) {
   EXPECT_CALL(*mock_odbc_helper, CheckResult(testing::_, testing::_, testing::_, testing::_))
       .WillRepeatedly(Return(false));

   EXPECT_THROW(ClusterTopologyMonitor(
     cluster_id,
     topology_map,
     conn_str,
     mock_odbc_helper,
     mock_query_helper,
     ignore_topology_request_ns,
     high_refresh_rate_ns,
     refresh_rate_ns
   ), std::exception);
}

TEST_F(ClusterTopologyMonitorTest, run_panic_no_cached_topology_writer_conn) {
    EXPECT_CALL(*mock_odbc_helper, Cleanup(testing::_, testing::_, testing::_))
        .Times(testing::AtLeast(0));
    EXPECT_CALL(*mock_odbc_helper, CheckConnection(testing::_))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*mock_odbc_helper, CheckResult(testing::_, testing::_, testing::_, testing::_))
        .WillRepeatedly(Return(true));
    // First connection (open_any_conn_GetHosts) is the writer
    // Note, this query should only return a string if the connected one is the writer
    EXPECT_CALL(*mock_query_helper, GetWriterId(testing::_))
        .WillOnce(Return("writer"))
        .WillRepeatedly(Return(""));

    std::vector<HostInfo> topology;
    topology.push_back(HostInfo("writer.server.com", 1234, UP, true, nullptr));
    topology.push_back(HostInfo("reader_a.server.com", 1234, UP, false, nullptr));
    topology.push_back(HostInfo("reader_b.server.com", 1234, UP, false, nullptr));

    EXPECT_CALL(*mock_query_helper, QueryTopology(testing::_))
        .WillRepeatedly(Return(topology));

    // Create monitor, starts main thread
    monitor = std::make_shared<ClusterTopologyMonitor>(
    cluster_id,
    topology_map,
    conn_str,
    mock_odbc_helper,
    mock_query_helper,
    ignore_topology_request_ns,
    high_refresh_rate_ns,
    refresh_rate_ns
    );
    monitor->StartMonitor();

    // Sleep to give time for topology to update
    std::this_thread::sleep_for(std::chrono::seconds(sleep_duration_sec));

    // Check if topology is updated by main thread / open_any_conn_GetHosts
    EXPECT_EQ(1, topology_map->Size());
}

// Should do the same as above, except internally will not mark conn as writer
// or create the writer host info
TEST_F(ClusterTopologyMonitorTest, run_panic_no_cached_topology_init_reader_conn) {
    EXPECT_CALL(*mock_odbc_helper, Cleanup(testing::_, testing::_, testing::_))
        .Times(testing::AtLeast(0));
    EXPECT_CALL(*mock_odbc_helper, CheckConnection(testing::_))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*mock_odbc_helper, CheckResult(testing::_, testing::_, testing::_, testing::_))
        .WillRepeatedly(Return(true));
    // First connection (open_any_conn_GetHosts) is a reader
    // Empty string represents that query did not return result, meaning the connection is not the writer
    EXPECT_CALL(*mock_query_helper, GetWriterId(testing::_))
        .WillOnce(Return("")) // First connection by main thread, open_any_conn_GetHosts
        .WillOnce(Return("writer")) // Node thread connects to writer
        .WillRepeatedly(Return(""));;

    std::vector<HostInfo> topology;
    topology.push_back(HostInfo("writer.server.com", 1234, UP, true, nullptr));
    topology.push_back(HostInfo("reader_a.server.com", 1234, UP, false, nullptr));
    topology.push_back(HostInfo("reader_b.server.com", 1234, UP, false, nullptr));

    EXPECT_CALL(*mock_query_helper, QueryTopology(testing::_))
        .WillRepeatedly(Return(topology));

    // Create monitor, starts main thread
    monitor = std::make_shared<ClusterTopologyMonitor>(
    cluster_id,
    topology_map,
    conn_str,
    mock_odbc_helper,
    mock_query_helper,
    ignore_topology_request_ns,
    high_refresh_rate_ns,
    refresh_rate_ns
    );
    monitor->StartMonitor();

    // Sleep to give time for topology to update
    std::this_thread::sleep_for(std::chrono::seconds(sleep_duration_sec));

    // Check if topology is updated by main thread / open_any_conn_GetHosts
    EXPECT_EQ(1, topology_map->Size());
}

TEST_F(ClusterTopologyMonitorTest, run_panic_with_cached_hosts) {
    std::vector<HostInfo> topology;
    topology.push_back(HostInfo("writer.server.com", 1234, UP, true, nullptr));
    topology.push_back(HostInfo("reader_a.server.com", 1234, UP, false, nullptr));
    topology.push_back(HostInfo("reader_b.server.com", 1234, UP, false, nullptr));
    topology_map->Put(cluster_id, topology);

    EXPECT_CALL(*mock_odbc_helper, Cleanup(testing::_, testing::_, testing::_))
        .Times(testing::AtLeast(0));
    EXPECT_CALL(*mock_odbc_helper, CheckConnection(testing::_))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*mock_odbc_helper, CheckResult(testing::_, testing::_, testing::_, testing::_))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*mock_query_helper, QueryTopology(testing::_))
        .WillRepeatedly(Return(topology));
    EXPECT_CALL(*mock_query_helper, GetWriterId(testing::_))
        .WillOnce(Return("writer"))
        .WillRepeatedly(Return(""));

    // Create monitor, starts main thread
    monitor = std::make_shared<ClusterTopologyMonitor>(
        cluster_id,
        topology_map,
        conn_str,
        mock_odbc_helper,
        mock_query_helper,
        ignore_topology_request_ns,
        high_refresh_rate_ns,
        refresh_rate_ns
    );
    monitor->StartMonitor();

    // Sleep to give time for topology to update
    std::this_thread::sleep_for(std::chrono::seconds(sleep_duration_sec));

    // Check that topology did not increase in size or decrease
    EXPECT_EQ(1, topology_map->Size());
}
