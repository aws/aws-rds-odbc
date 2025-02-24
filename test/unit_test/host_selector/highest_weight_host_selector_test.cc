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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <highest_weight_host_selector.h>

namespace {
    constexpr int base_port = 1234;
    HostInfo reader_host_info_a("reader_a", base_port, UP, false, nullptr, HostInfo::DEFAULT_WEIGHT - 2);
    HostInfo reader_host_info_b("reader_b", base_port, UP, false, nullptr, HostInfo::DEFAULT_WEIGHT - 1);
    HostInfo reader_host_info_c("reader_c", base_port, UP, false, nullptr, HostInfo::DEFAULT_WEIGHT - 1);
    HostInfo reader_host_info_down_a("reader_down_a", base_port, DOWN, false, nullptr, HostInfo::DEFAULT_WEIGHT);
    HostInfo reader_host_info_down_b("reader_down_b", base_port, DOWN, false, nullptr, HostInfo::DEFAULT_WEIGHT + 1);
    HostInfo writer_host_info_a("writer_a", base_port, UP, true, nullptr, HostInfo::DEFAULT_WEIGHT - 2);
    HostInfo writer_host_info_b("writer_b", base_port, UP, true, nullptr, HostInfo::DEFAULT_WEIGHT - 1);
    HostInfo writer_host_info_c("writer_c", base_port, UP, true, nullptr, HostInfo::DEFAULT_WEIGHT - 1);
    HostInfo writer_host_info_down_a("writer_down_a", base_port, DOWN, true, nullptr, HostInfo::DEFAULT_WEIGHT);
    HostInfo writer_host_info_down_b("writer_down_b", base_port, DOWN, true, nullptr, HostInfo::DEFAULT_WEIGHT + 1);
    std::vector<HostInfo> all_hosts = {
        reader_host_info_a,
        reader_host_info_b,
        reader_host_info_c,
        reader_host_info_down_a,
        reader_host_info_down_b,
        writer_host_info_a,
        writer_host_info_b,
        writer_host_info_c,
        writer_host_info_down_a,
        writer_host_info_down_b
    };
    std::unordered_map<std::string, std::string> empty_map;
}

class HighestWeightHostSelectorTest : public testing::Test {
protected:
    // Runs once per suite
    static void SetUpTestSuite() {}
    static void TearDownTestSuite() {}
    // Runs per test case
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(HighestWeightHostSelectorTest, get_highest_reader) {
    HighestWeightHostSelector host_selector;
    HostInfo host_info = host_selector.get_host(all_hosts, false, empty_map);
    EXPECT_EQ(reader_host_info_b.get_host(), host_info.get_host());
}

TEST_F(HighestWeightHostSelectorTest, get_highest_writer) {
    HighestWeightHostSelector host_selector;
    HostInfo host_info = host_selector.get_host(all_hosts, true, empty_map);
    EXPECT_EQ(writer_host_info_b.get_host(), host_info.get_host());
}

TEST_F(HighestWeightHostSelectorTest, get_reader_fail_all_writers) {
    HighestWeightHostSelector host_selector;
    std::vector<HostInfo> hosts = {
        writer_host_info_a,
        writer_host_info_b,
        writer_host_info_c,
        writer_host_info_down_a
    };
    EXPECT_THROW(host_selector.get_host(hosts, false, empty_map), std::runtime_error);
}

TEST_F(HighestWeightHostSelectorTest, get_writer_fail_all_readers) {
    HighestWeightHostSelector host_selector;
    std::vector<HostInfo> hosts = {
        reader_host_info_a,
        reader_host_info_b,
        reader_host_info_c,
        reader_host_info_down_a
    };
    EXPECT_THROW(host_selector.get_host(hosts, true, empty_map), std::runtime_error);
}

TEST_F(HighestWeightHostSelectorTest, get_reader_all_down) {
    HighestWeightHostSelector host_selector;
    std::vector<HostInfo> hosts = {
        reader_host_info_down_a,
        reader_host_info_down_b
    };
    EXPECT_THROW(host_selector.get_host(hosts, false, empty_map), std::runtime_error);
}

TEST_F(HighestWeightHostSelectorTest, get_writer_all_down) {
    HighestWeightHostSelector host_selector;
    std::vector<HostInfo> hosts = {
        writer_host_info_down_a,
        writer_host_info_down_b
    };
    EXPECT_THROW(host_selector.get_host(hosts, true, empty_map), std::runtime_error);
}

TEST_F(HighestWeightHostSelectorTest, get_reader_no_hosts) {
    HighestWeightHostSelector host_selector;
    std::vector<HostInfo> hosts = {};
    EXPECT_THROW(host_selector.get_host(hosts, false, empty_map), std::runtime_error);
}

TEST_F(HighestWeightHostSelectorTest, get_writer_no_hosts) {
    HighestWeightHostSelector host_selector;
    std::vector<HostInfo> hosts = {};
    EXPECT_THROW(host_selector.get_host(hosts, true, empty_map), std::runtime_error);
}
