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
