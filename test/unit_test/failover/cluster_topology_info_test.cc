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

#include <gtest/gtest.h>
#include <cluster_topology_info.h>
#include <host_info.h>
#include <simple_host_availability_strategy.h>

#include "../mock_objects.h"

using ::testing::Return;

namespace {
  const std::string base_host_string = "hostName";
  const int base_port = 1234;
  std::shared_ptr<SimpleHostAvailabilityStrategy> simple_host_availability_strategy =
    std::make_shared<SimpleHostAvailabilityStrategy>();
}

class ClusterTopologyInfoTest : public testing::Test {
  protected:
    // Runs once per suite
    static void SetUpTestSuite() {}
    static void TearDownTestSuite() {}
    // Runs per test case
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(ClusterTopologyInfoTest, getNextWriter) {
  std::shared_ptr<HostInfo> writer_host_info =
    std::make_shared<HostInfo>(base_host_string, base_port, HOST_STATE::UP, true, simple_host_availability_strategy);
  std::shared_ptr<HostInfo> reader_host_info_a =
    std::make_shared<HostInfo>(base_host_string, base_port, HOST_STATE::UP, false, simple_host_availability_strategy);
  std::shared_ptr<HostInfo> reader_host_info_b =
    std::make_shared<HostInfo>(base_host_string, base_port, HOST_STATE::UP, false, simple_host_availability_strategy);
  std::shared_ptr<ClusterTopologyInfo> cluster_topology_info = std::make_shared<ClusterTopologyInfo>();
  cluster_topology_info->add_host(writer_host_info);
  cluster_topology_info->add_host(reader_host_info_a);
  cluster_topology_info->add_host(reader_host_info_b);
  std::shared_ptr<HostInfo> host_info = cluster_topology_info->get_writer();
  EXPECT_EQ(writer_host_info, host_info);
}

TEST_F(ClusterTopologyInfoTest, getNextWriter_empty) {
  std::shared_ptr<ClusterTopologyInfo> cluster_topology_info = std::make_shared<ClusterTopologyInfo>();
  EXPECT_THROW(cluster_topology_info->get_writer(), std::runtime_error);
}
