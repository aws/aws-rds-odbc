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

#include "cluster_topology_info.h"

#include <gtest/gtest.h>

#include "../mock_objects.h"
#include "host_info.h"
#include "simple_host_availability_strategy.h"

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

TEST_F(ClusterTopologyInfoTest, get_next_writer) {
  std::shared_ptr<HostInfo> writer_host_info =
    std::make_shared<HostInfo>(base_host_string, base_port, HOST_STATE::UP, true, simple_host_availability_strategy);
  std::shared_ptr<HostInfo> reader_host_info_a =
    std::make_shared<HostInfo>(base_host_string, base_port, HOST_STATE::UP, false, simple_host_availability_strategy);
  std::shared_ptr<HostInfo> reader_host_info_b =
    std::make_shared<HostInfo>(base_host_string, base_port, HOST_STATE::UP, false, simple_host_availability_strategy);
  std::shared_ptr<ClusterTopologyInfo> cluster_topology_info = std::make_shared<ClusterTopologyInfo>();
  cluster_topology_info->AddHost(writer_host_info);
  cluster_topology_info->AddHost(reader_host_info_a);
  cluster_topology_info->AddHost(reader_host_info_b);
  std::shared_ptr<HostInfo> host_info = cluster_topology_info->GetWriter();
  EXPECT_EQ(writer_host_info, host_info);
}

TEST_F(ClusterTopologyInfoTest, get_next_writer_empty) {
  std::shared_ptr<ClusterTopologyInfo> cluster_topology_info = std::make_shared<ClusterTopologyInfo>();
  EXPECT_THROW(cluster_topology_info->GetWriter(), std::runtime_error);
}
