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

#include "host_info.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "simple_host_availability_strategy.h"

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

TEST_F(HostInfoTest, GetHost) {
  EXPECT_EQ(host, hostInfo->GetHost());
}

TEST_F(HostInfoTest, GetPort) {
  EXPECT_EQ(port, hostInfo->GetPort());
}

TEST_F(HostInfoTest, GetHostPortPair) {
  EXPECT_EQ(hostPortPair, hostInfo->GetHostPortPair());
}

TEST_F(HostInfoTest, EqualHostPortPair) {
  HostInfo hostInfoHostPortMatches(host, port, UP, false, simple_host_availability_strategy);
  EXPECT_TRUE(hostInfo->EqualHostPortPair(hostInfoHostPortMatches));

  HostInfo hostInfoHostDoesNotMatch(host + "DoesNotMatch", port, UP, false, simple_host_availability_strategy);
  EXPECT_FALSE(hostInfo->EqualHostPortPair(hostInfoHostDoesNotMatch));

  HostInfo hostInfoPortDoesNotMatch(host, port + 1, UP, false, simple_host_availability_strategy);
  EXPECT_FALSE(hostInfo->EqualHostPortPair(hostInfoPortDoesNotMatch));
}

TEST_F(HostInfoTest, GetHostState) {
  EXPECT_EQ(UP, hostInfo->GetHostState());
}

TEST_F(HostInfoTest, GetWeight) {
  EXPECT_EQ(HostInfo::DEFAULT_WEIGHT, hostInfo->GetWeight());
}

TEST_F(HostInfoTest, set_host_state) {
  EXPECT_EQ(UP, hostInfo->GetHostState());
  hostInfo->SetHostState(DOWN);
  EXPECT_EQ(DOWN, hostInfo->GetHostState());
}

TEST_F(HostInfoTest, IsHostUp) {
  EXPECT_TRUE(hostInfo->IsHostUp());
}

TEST_F(HostInfoTest, IsHostDown) {
  EXPECT_FALSE(hostInfo->IsHostDown());
}

TEST_F(HostInfoTest, IsHostWriter) {
  EXPECT_FALSE(hostInfo->IsHostWriter());
}

TEST_F(HostInfoTest, MarkAsWriter) {
  EXPECT_FALSE(hostInfo->IsHostWriter());
  hostInfo->MarkAsWriter(true);
  EXPECT_TRUE(hostInfo->IsHostWriter());
  hostInfo->MarkAsWriter(false);
  EXPECT_FALSE(hostInfo->IsHostWriter());
}

TEST_F(HostInfoTest, GetHostAvailabilityStrategy) {
  EXPECT_EQ(simple_host_availability_strategy, hostInfo->GetHostAvailabilityStrategy());
}

TEST_F(HostInfoTest, SetHostAvailabilityStrategy) {
  hostInfo->SetHostAvailabilityStrategy(nullptr);
  EXPECT_EQ(nullptr, hostInfo->GetHostAvailabilityStrategy());

  hostInfo->SetHostAvailabilityStrategy(simple_host_availability_strategy);
  EXPECT_EQ(simple_host_availability_strategy, hostInfo->GetHostAvailabilityStrategy());
}
