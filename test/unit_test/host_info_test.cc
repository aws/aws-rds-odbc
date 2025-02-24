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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <host_info.h>
#include <simple_host_availability_strategy.h>

namespace {
  const std::string host = "hostName";
  int port = 1234;
  const std::string hostPortPair = host + ":" + std::to_string(port);
  std::shared_ptr<SimpleHostAvailabilityStrategy> simple_host_availability_strategy = 
    std::make_shared<SimpleHostAvailabilityStrategy>();
}

class HostInfoTest : public testing::Test {
  protected:
    // Runs once per suite
    static void SetUpTestSuite() {}
    static void TearDownTestSuite() {}
    // Runs per test case
    void SetUp() override {
        hostInfo = std::make_shared<HostInfo>(host, port, UP, false, simple_host_availability_strategy);
    }
    void TearDown() override {}

    std::shared_ptr<HostInfo> hostInfo;
};

TEST_F(HostInfoTest, get_host) {
  EXPECT_EQ(host, hostInfo->get_host());
}

TEST_F(HostInfoTest, get_port) {
  EXPECT_EQ(port, hostInfo->get_port());
}

TEST_F(HostInfoTest, get_host_port_pair) {
  EXPECT_EQ(hostPortPair, hostInfo->get_host_port_pair());
}

TEST_F(HostInfoTest, equal_host_port_pair) {
  HostInfo hostInfoHostPortMatches(host, port, UP, false, simple_host_availability_strategy);
  EXPECT_TRUE(hostInfo->equal_host_port_pair(hostInfoHostPortMatches));

  HostInfo hostInfoHostDoesNotMatch(host + "DoesNotMatch", port, UP, false, simple_host_availability_strategy);
  EXPECT_FALSE(hostInfo->equal_host_port_pair(hostInfoHostDoesNotMatch));

  HostInfo hostInfoPortDoesNotMatch(host, port + 1, UP, false, simple_host_availability_strategy);
  EXPECT_FALSE(hostInfo->equal_host_port_pair(hostInfoPortDoesNotMatch));
}

TEST_F(HostInfoTest, get_host_state) {
  EXPECT_EQ(UP, hostInfo->get_host_state());
}

TEST_F(HostInfoTest, get_weight) {
  EXPECT_EQ(HostInfo::DEFAULT_WEIGHT, hostInfo->get_weight());
}

TEST_F(HostInfoTest, set_host_state) {
  EXPECT_EQ(UP, hostInfo->get_host_state());
  hostInfo->set_host_state(DOWN);
  EXPECT_EQ(DOWN, hostInfo->get_host_state());
}

TEST_F(HostInfoTest, is_host_up) {
  EXPECT_TRUE(hostInfo->is_host_up());
}

TEST_F(HostInfoTest, is_host_down) {
  EXPECT_FALSE(hostInfo->is_host_down());
}

TEST_F(HostInfoTest, is_host_writer) {
  EXPECT_FALSE(hostInfo->is_host_writer());
}

TEST_F(HostInfoTest, mark_as_writer) {
  EXPECT_FALSE(hostInfo->is_host_writer());
  hostInfo->mark_as_writer(true);
  EXPECT_TRUE(hostInfo->is_host_writer());
  hostInfo->mark_as_writer(false);
  EXPECT_FALSE(hostInfo->is_host_writer());
}

TEST_F(HostInfoTest, get_host_availability_strategy) {
  EXPECT_EQ(simple_host_availability_strategy, hostInfo->get_host_availability_strategy());
}

TEST_F(HostInfoTest, set_host_availability_strategy) {
  hostInfo->set_host_availability_strategy(nullptr);
  EXPECT_EQ(nullptr, hostInfo->get_host_availability_strategy());

  hostInfo->set_host_availability_strategy(simple_host_availability_strategy);
  EXPECT_EQ(simple_host_availability_strategy, hostInfo->get_host_availability_strategy());
}
