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
