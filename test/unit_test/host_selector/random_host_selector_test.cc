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

#include <random_host_selector.h>

namespace {
    constexpr int base_port = 1234;
    HostInfo writer_host_info_a("writer_a", base_port, UP, true, nullptr);
    HostInfo writer_host_info_b("writer_b", base_port, UP, true, nullptr);
    HostInfo writer_host_info_down("writer_down", base_port, DOWN, true, nullptr);
    HostInfo reader_host_info_a("reader_a", base_port, UP, false, nullptr);
    HostInfo reader_host_info_b("reader_b", base_port, UP, false, nullptr);
    HostInfo reader_host_info_down("reader_down", base_port, DOWN, false, nullptr);
    std::unordered_map<std::string, std::string> empty_map;
}

class RandomHostSelectorTest : public testing::Test {
  protected:
    // Runs once per suite
    static void SetUpTestSuite() {}
    static void TearDownTestSuite() {}
    // Runs per test case
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(RandomHostSelectorTest, get_writer) {
    RandomHostSelector host_selector;
    std::vector<HostInfo> hosts = {writer_host_info_a, reader_host_info_a, reader_host_info_b};
    HostInfo host_info = host_selector.get_host(hosts, true, empty_map);
    EXPECT_EQ(writer_host_info_a.get_host(), host_info.get_host());
}

TEST_F(RandomHostSelectorTest, get_live_writer) {
    RandomHostSelector host_selector;
    std::vector<HostInfo> hosts = {writer_host_info_a, writer_host_info_down};
    HostInfo host_info = host_selector.get_host(hosts, true, empty_map);
    EXPECT_EQ(writer_host_info_a.get_host(), host_info.get_host());
}

TEST_F(RandomHostSelectorTest, get_writer_from_many) {
    RandomHostSelector host_selector;
    std::vector<HostInfo> hosts = {writer_host_info_a, writer_host_info_b, writer_host_info_down,
        reader_host_info_a, reader_host_info_b, reader_host_info_down};
    HostInfo host_info = host_selector.get_host(hosts, true, empty_map);
    EXPECT_TRUE(host_info.is_host_writer());
    EXPECT_TRUE(host_info.is_host_up());
}

TEST_F(RandomHostSelectorTest, get_writer_fail_all_readers) {
    RandomHostSelector host_selector;
    std::vector<HostInfo> hosts = {reader_host_info_a, reader_host_info_b};
    EXPECT_THROW(host_selector.get_host(hosts, true, empty_map), std::runtime_error);
}

TEST_F(RandomHostSelectorTest, get_reader_one_down) {
    RandomHostSelector host_selector;
    std::vector<HostInfo> hosts = {reader_host_info_a, reader_host_info_down};
    HostInfo host_info = host_selector.get_host(hosts, false, empty_map);
    EXPECT_EQ(reader_host_info_a.get_host(), host_info.get_host());
}

TEST_F(RandomHostSelectorTest, no_hosts) {
    RandomHostSelector host_selector;
    std::vector<HostInfo> hosts = {};
    EXPECT_THROW(host_selector.get_host(hosts, false, empty_map), std::runtime_error);
}
