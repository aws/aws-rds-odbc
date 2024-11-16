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

#include <round_robin_host_selector.h>
#include <simple_host_availability_strategy.h>

namespace {
    const int base_port = 1234;
    HostInfo writer_host_info_a("writer", base_port, HOST_STATE::UP, true, nullptr, 1);
    HostInfo reader_host_info_a("reader_a", base_port, HOST_STATE::UP, false, nullptr, 1);
    HostInfo reader_host_info_b("reader_b", base_port, HOST_STATE::UP, false, nullptr, 1);
    HostInfo reader_host_info_c("reader_c", base_port, HOST_STATE::UP, false, nullptr, 1);
    HostInfo reader_host_info_down("reader_down", base_port, HOST_STATE::DOWN, false, nullptr, 1);
}

class RoundRobinHostSelectorTest : public testing::Test {
  protected:
    // Runs once per suite
    static void SetUpTestSuite() {}
    static void TearDownTestSuite() {}
    // Runs per test case
    void SetUp() override {
        RoundRobinHostSelector::clear_cache();
    }
    void TearDown() override {}
};

TEST_F(RoundRobinHostSelectorTest, get_writer) {
    RoundRobinHostSelector host_selector;
    std::unordered_map<std::string, std::string> props;
    std::vector<HostInfo> hosts = {writer_host_info_a, reader_host_info_a, reader_host_info_b};
    HostInfo host_info = host_selector.get_host(hosts, true, props);
    EXPECT_EQ(writer_host_info_a.get_host(), host_info.get_host());
}

TEST_F(RoundRobinHostSelectorTest, get_writer_fail_all_readers) {
    RoundRobinHostSelector host_selector;
    std::unordered_map<std::string, std::string> props;
    std::vector<HostInfo> hosts = {reader_host_info_a, reader_host_info_b};
    EXPECT_THROW(host_selector.get_host(hosts, true, props), std::runtime_error);
}

TEST_F(RoundRobinHostSelectorTest, get_reader_one_down) {
    RoundRobinHostSelector host_selector;
    std::unordered_map<std::string, std::string> props;
    std::vector<HostInfo> hosts = {reader_host_info_a, reader_host_info_down};
    HostInfo host_info = host_selector.get_host(hosts, false, props);
    EXPECT_EQ(reader_host_info_a.get_host(), host_info.get_host());
}

TEST_F(RoundRobinHostSelectorTest, no_hosts) {
    RoundRobinHostSelector host_selector;
    std::unordered_map<std::string, std::string> props;
    std::vector<HostInfo> hosts = {};
    EXPECT_THROW(host_selector.get_host(hosts, false, props), std::runtime_error);
}

TEST_F(RoundRobinHostSelectorTest, get_first_reader) {
    RoundRobinHostSelector host_selector;
    std::unordered_map<std::string, std::string> props;
    std::vector<HostInfo> hosts = {reader_host_info_a, reader_host_info_b, reader_host_info_c};
    HostInfo host_info = host_selector.get_host(hosts, false, props);
    EXPECT_EQ(reader_host_info_a.get_host(), host_info.get_host());
}

TEST_F(RoundRobinHostSelectorTest, get_round_robin_readers_default) {
    RoundRobinHostSelector host_selector;
    std::unordered_map<std::string, std::string> props;
    std::vector<HostInfo> hosts = {reader_host_info_a, reader_host_info_b};

    HostInfo host_info = host_selector.get_host(hosts, false, props);
    EXPECT_EQ(reader_host_info_a.get_host(), host_info.get_host());

    host_info = host_selector.get_host(hosts, false, props);
    EXPECT_EQ(reader_host_info_b.get_host(), host_info.get_host());

    host_info = host_selector.get_host(hosts, false, props);
    EXPECT_EQ(reader_host_info_a.get_host(), host_info.get_host());
}

TEST_F(RoundRobinHostSelectorTest, get_round_robin_readers_weighted) {
    RoundRobinHostSelector host_selector;
    std::unordered_map<std::string, std::string> props;
    props[round_robin_property::HOST_WEIGHT_KEY] = std::string(
        reader_host_info_a.get_host() + ":2"
         + "," + reader_host_info_b.get_host() + ":1"
    );
    std::vector<HostInfo> hosts = {reader_host_info_a, reader_host_info_b};

    HostInfo host_info = host_selector.get_host(hosts, false, props);
    EXPECT_EQ(reader_host_info_a.get_host(), host_info.get_host());
    host_info = host_selector.get_host(hosts, false, props);
    EXPECT_EQ(reader_host_info_a.get_host(), host_info.get_host());

    host_info = host_selector.get_host(hosts, false, props);
    EXPECT_EQ(reader_host_info_b.get_host(), host_info.get_host());

    host_info = host_selector.get_host(hosts, false, props);
    EXPECT_EQ(reader_host_info_a.get_host(), host_info.get_host());
}

TEST_F(RoundRobinHostSelectorTest, get_round_robin_readers_weighted_change) {
    RoundRobinHostSelector host_selector;
    std::unordered_map<std::string, std::string> props;
    props[round_robin_property::HOST_WEIGHT_KEY] = std::string(
        reader_host_info_a.get_host() + ":2"
         + "," + reader_host_info_b.get_host() + ":1"
    );
    std::vector<HostInfo> hosts = {reader_host_info_a, reader_host_info_b};

    HostInfo host_info = host_selector.get_host(hosts, false, props);
    EXPECT_EQ(reader_host_info_a.get_host(), host_info.get_host());
    host_info = host_selector.get_host(hosts, false, props);
    EXPECT_EQ(reader_host_info_a.get_host(), host_info.get_host());

    host_info = host_selector.get_host(hosts, false, props);
    EXPECT_EQ(reader_host_info_b.get_host(), host_info.get_host());

    host_info = host_selector.get_host(hosts, false, props);
    EXPECT_EQ(reader_host_info_a.get_host(), host_info.get_host());

    std::unordered_map<std::string, std::string> updated_props;
    updated_props[round_robin_property::HOST_WEIGHT_KEY] = std::string(
        reader_host_info_a.get_host() + ":1"
         + "," + reader_host_info_b.get_host() + ":2"
    );

    host_info = host_selector.get_host(hosts, false, updated_props);
    EXPECT_EQ(reader_host_info_a.get_host(), host_info.get_host());

    host_info = host_selector.get_host(hosts, false, updated_props);
    EXPECT_EQ(reader_host_info_b.get_host(), host_info.get_host());
    host_info = host_selector.get_host(hosts, false, updated_props);
    EXPECT_EQ(reader_host_info_b.get_host(), host_info.get_host());

    host_info = host_selector.get_host(hosts, false, updated_props);
    EXPECT_EQ(reader_host_info_a.get_host(), host_info.get_host());
}

TEST_F(RoundRobinHostSelectorTest, set_round_robin_weight) {
    RoundRobinHostSelector host_selector;
    std::unordered_map<std::string, std::string> props;
    std::string expected_weight_prop(
        reader_host_info_a.get_host() + ":" + std::to_string(reader_host_info_a.get_weight())
         + "," + reader_host_info_b.get_host() + ":" + std::to_string(reader_host_info_b.get_weight())
    );
    std::vector<HostInfo> hosts = {reader_host_info_a, reader_host_info_b};
    RoundRobinHostSelector::set_round_robin_weight(hosts, props);

    std::string weight_prop = props.at(round_robin_property::HOST_WEIGHT_KEY);
    EXPECT_STREQ(expected_weight_prop.c_str(), weight_prop.c_str());

    HostInfo host_info = host_selector.get_host(hosts, false, props);
    EXPECT_EQ(reader_host_info_a.get_host(), host_info.get_host());

    host_info = host_selector.get_host(hosts, false, props);
    EXPECT_EQ(reader_host_info_b.get_host(), host_info.get_host());

    host_info = host_selector.get_host(hosts, false, props);
    EXPECT_EQ(reader_host_info_a.get_host(), host_info.get_host());
}

TEST_F(RoundRobinHostSelectorTest, invalid_host_weight_empty_host) {
    RoundRobinHostSelector host_selector;
    std::unordered_map<std::string, std::string> props;
    props[round_robin_property::HOST_WEIGHT_KEY] = std::string(
        ":" + std::to_string(reader_host_info_a.get_weight())
         + "," + reader_host_info_b.get_host() + ":" + std::to_string(reader_host_info_b.get_weight())
    );
    std::vector<HostInfo> hosts = {reader_host_info_a, reader_host_info_b};
    EXPECT_THROW(host_selector.get_host(hosts, false, props), std::runtime_error);
}

TEST_F(RoundRobinHostSelectorTest, invalid_host_weight_empty_weight) {
    RoundRobinHostSelector host_selector;
    std::unordered_map<std::string, std::string> props;
    props[round_robin_property::HOST_WEIGHT_KEY] = std::string(
        reader_host_info_a.get_host() + ":"
         + "," + reader_host_info_b.get_host() + ":" + std::to_string(reader_host_info_b.get_weight())
    );
    std::vector<HostInfo> hosts = {reader_host_info_a, reader_host_info_b};
    EXPECT_THROW(host_selector.get_host(hosts, false, props), std::runtime_error);
}

TEST_F(RoundRobinHostSelectorTest, invalid_host_weight_zero_weight) {
    RoundRobinHostSelector host_selector;
    std::unordered_map<std::string, std::string> props;
    props[round_robin_property::HOST_WEIGHT_KEY] = std::string(
        reader_host_info_a.get_host() + ":0"
         + "," + reader_host_info_b.get_host() + ":" + std::to_string(reader_host_info_b.get_weight())
    );
    std::vector<HostInfo> hosts = {reader_host_info_a, reader_host_info_b};
    EXPECT_THROW(host_selector.get_host(hosts, false, props), std::runtime_error);
}

TEST_F(RoundRobinHostSelectorTest, invalid_host_weight_negative_weight) {
    RoundRobinHostSelector host_selector;
    std::unordered_map<std::string, std::string> props;
    props[round_robin_property::HOST_WEIGHT_KEY] = std::string(
        reader_host_info_a.get_host() + ":-1"
         + "," + reader_host_info_b.get_host() + ":" + std::to_string(reader_host_info_b.get_weight())
    );
    std::vector<HostInfo> hosts = {reader_host_info_a, reader_host_info_b};
    EXPECT_THROW(host_selector.get_host(hosts, false, props), std::runtime_error);
}

TEST_F(RoundRobinHostSelectorTest, invalid_host_weight_float_weight) {
    RoundRobinHostSelector host_selector;
    std::unordered_map<std::string, std::string> props;
    props[round_robin_property::HOST_WEIGHT_KEY] = std::string(
        reader_host_info_a.get_host() + ":1.1"
         + "," + reader_host_info_b.get_host() + ":" + std::to_string(reader_host_info_b.get_weight())
    );
    std::vector<HostInfo> hosts = {reader_host_info_a, reader_host_info_b};
    EXPECT_THROW(host_selector.get_host(hosts, false, props), std::runtime_error);
}

TEST_F(RoundRobinHostSelectorTest, invalid_host_weight_non_int_weight) {
    RoundRobinHostSelector host_selector;
    std::unordered_map<std::string, std::string> props;
    props[round_robin_property::HOST_WEIGHT_KEY] = std::string(
        reader_host_info_a.get_host() + ":one"
         + "," + reader_host_info_b.get_host() + ":" + std::to_string(reader_host_info_b.get_weight())
    );
    std::vector<HostInfo> hosts = {reader_host_info_a, reader_host_info_b};
    EXPECT_THROW(host_selector.get_host(hosts, false, props), std::runtime_error);
}

TEST_F(RoundRobinHostSelectorTest, invalid_host_weight_format) {
    RoundRobinHostSelector host_selector;
    std::unordered_map<std::string, std::string> props;
    props[round_robin_property::HOST_WEIGHT_KEY] = std::string(
        reader_host_info_a.get_host() + ":111:" + std::to_string(reader_host_info_a.get_weight())
         + "," + reader_host_info_b.get_host() + ":" + std::to_string(reader_host_info_b.get_weight())
    );
    std::vector<HostInfo> hosts = {reader_host_info_a, reader_host_info_b};
    EXPECT_THROW(host_selector.get_host(hosts, false, props), std::runtime_error);
}

TEST_F(RoundRobinHostSelectorTest, invalid_default_zero) {
    RoundRobinHostSelector host_selector;
    std::unordered_map<std::string, std::string> props;
    props[round_robin_property::DEFAULT_WEIGHT_KEY] = std::string("0");
    std::vector<HostInfo> hosts = {reader_host_info_a, reader_host_info_b};
    EXPECT_THROW(host_selector.get_host(hosts, false, props), std::runtime_error);
}

TEST_F(RoundRobinHostSelectorTest, invalid_default_negative) {
    RoundRobinHostSelector host_selector;
    std::unordered_map<std::string, std::string> props;
    props[round_robin_property::DEFAULT_WEIGHT_KEY] = std::string("-100");
    std::vector<HostInfo> hosts = {reader_host_info_a, reader_host_info_b};
    EXPECT_THROW(host_selector.get_host(hosts, false, props), std::runtime_error);
}

TEST_F(RoundRobinHostSelectorTest, invalid_default_float) {
    RoundRobinHostSelector host_selector;
    std::unordered_map<std::string, std::string> props;
    props[round_robin_property::DEFAULT_WEIGHT_KEY] = std::string("1.1");
    std::vector<HostInfo> hosts = {reader_host_info_a, reader_host_info_b};
    EXPECT_THROW(host_selector.get_host(hosts, false, props), std::runtime_error);
}

TEST_F(RoundRobinHostSelectorTest, invalid_default_non_int) {
    RoundRobinHostSelector host_selector;
    std::unordered_map<std::string, std::string> props;
    props[round_robin_property::DEFAULT_WEIGHT_KEY] = std::string("one");
    std::vector<HostInfo> hosts = {reader_host_info_a, reader_host_info_b};
    EXPECT_THROW(host_selector.get_host(hosts, false, props), std::runtime_error);
}

