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
