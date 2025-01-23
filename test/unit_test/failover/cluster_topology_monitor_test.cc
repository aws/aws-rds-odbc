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

#include <gtest/gtest.h>
#include <cluster_topology_monitor.h>

#include "../mock_objects.h"

using ::testing::Return;

namespace {
  const std::string cluster_id = "cluster_test_id";
  const SQLTCHAR* conn_str = reinterpret_cast<SQLTCHAR*>(const_cast<char*>("DSN=Test_DSN;Server=cluster.server.com;"));
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
 // First connection (open_any_conn_get_hosts) is the writer
 // Note, this query should only return a string if the connected one is the writer
 EXPECT_CALL(*mock_query_helper, get_writer_id(testing::_))
     .WillOnce(Return("writer"))
     .WillRepeatedly(Return(""));

 std::vector<HostInfo> topology;
 topology.push_back(HostInfo("writer.server.com", 1234, UP, true, nullptr));
 topology.push_back(HostInfo("reader_a.server.com", 1234, UP, false, nullptr));
 topology.push_back(HostInfo("reader_b.server.com", 1234, UP, false, nullptr));

 EXPECT_CALL(*mock_query_helper, query_topology(testing::_))
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
 monitor->start_monitor();

 // Sleep to give time for topology to update
 std::this_thread::sleep_for(std::chrono::seconds(sleep_duration_sec));

 // Check if topology is updated by main thread / open_any_conn_get_hosts
 EXPECT_EQ(1, topology_map->size());
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
 // First connection (open_any_conn_get_hosts) is a reader
 // Empty string represents that query did not return result, meaning the connection is not the writer
 EXPECT_CALL(*mock_query_helper, get_writer_id(testing::_))
     .WillOnce(Return("")) // First connection by main thread, open_any_conn_get_hosts
     .WillOnce(Return("writer")) // Node thread connects to writer
     .WillRepeatedly(Return(""));;

 std::vector<HostInfo> topology;
 topology.push_back(HostInfo("writer.server.com", 1234, UP, true, nullptr));
 topology.push_back(HostInfo("reader_a.server.com", 1234, UP, false, nullptr));
 topology.push_back(HostInfo("reader_b.server.com", 1234, UP, false, nullptr));

 EXPECT_CALL(*mock_query_helper, query_topology(testing::_))
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
 monitor->start_monitor();

 // Sleep to give time for topology to update
 std::this_thread::sleep_for(std::chrono::seconds(sleep_duration_sec));

 // Check if topology is updated by main thread / open_any_conn_get_hosts
 EXPECT_EQ(1, topology_map->size());
}

TEST_F(ClusterTopologyMonitorTest, run_panic_with_cached_hosts) {
 std::vector<HostInfo> topology;
 topology.push_back(HostInfo("writer.server.com", 1234, UP, true, nullptr));
 topology.push_back(HostInfo("reader_a.server.com", 1234, UP, false, nullptr));
 topology.push_back(HostInfo("reader_b.server.com", 1234, UP, false, nullptr));
 topology_map->put(cluster_id, topology);

 EXPECT_CALL(*mock_odbc_helper, Cleanup(testing::_, testing::_, testing::_))
     .Times(testing::AtLeast(0));
 EXPECT_CALL(*mock_odbc_helper, CheckConnection(testing::_))
     .WillRepeatedly(Return(true));
 EXPECT_CALL(*mock_odbc_helper, CheckResult(testing::_, testing::_, testing::_, testing::_))
     .WillRepeatedly(Return(true));
 EXPECT_CALL(*mock_query_helper, query_topology(testing::_))
     .WillRepeatedly(Return(topology));
 EXPECT_CALL(*mock_query_helper, get_writer_id(testing::_))
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
 monitor->start_monitor();

 // Sleep to give time for topology to update
 std::this_thread::sleep_for(std::chrono::seconds(sleep_duration_sec));

 // Check that topology did not increase in size or decrease
 EXPECT_EQ(1, topology_map->size());
}
