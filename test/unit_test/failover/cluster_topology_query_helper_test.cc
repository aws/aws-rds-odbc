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

#include <algorithm>
#include <string>

#include <cluster_topology_query_helper.h>

#include "../mock_objects.h"

using ::testing::Return;

class ClusterTopologyQueryHelperTest : public testing::Test {
protected:
    // Runs once per suite
    static void SetUpTestSuite() {}
    static void TearDownTestSuite() {}
    // Runs per test case
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(ClusterTopologyQueryHelperTest, create_host) {
    SQLTCHAR* node_id = AS_SQLCHAR("node_id");
    int port = 1;
    bool is_writer = false;
    float cpu_usage = 2;
    float replica_lag = 3;
    float weight = std::round(replica_lag) * 100L + std::round(cpu_usage);
    SQL_TIMESTAMP_STRUCT timestamp{2000, 01, 01, 01, 01, 01, 01};

    std::shared_ptr<ClusterTopologyQueryHelper> query_helper =
        std::make_shared<ClusterTopologyQueryHelper>(port, "?", "", "", "");

    HostInfo host = query_helper->create_host(node_id, false, cpu_usage, replica_lag, timestamp);
    EXPECT_EQ(std::string(AS_CHAR(node_id)), host.get_host());
    EXPECT_EQ(port, host.get_port());
    EXPECT_EQ(weight, host.get_weight());
    EXPECT_EQ(is_writer, host.is_host_writer());
}

TEST_F(ClusterTopologyQueryHelperTest, get_endpoint) {
    std::shared_ptr<ClusterTopologyQueryHelper> query_helper =
        std::make_shared<ClusterTopologyQueryHelper>(1234, "?", "", "", "");
    SQLTCHAR* input = AS_SQLCHAR("node-id");
    std::string expected = AS_CHAR(input);
    EXPECT_EQ(expected, query_helper->get_endpoint(input));
}

TEST_F(ClusterTopologyQueryHelperTest, get_endpoint_extra_values) {
    std::string endpoint_template = "?.other-values";
    std::shared_ptr<ClusterTopologyQueryHelper> query_helper =
        std::make_shared<ClusterTopologyQueryHelper>(1234, endpoint_template, "", "", "");
    SQLTCHAR* input = AS_SQLCHAR("node-id");
    std::string expected = std::string(endpoint_template).replace(endpoint_template.find("?"), 1, AS_CHAR(input));
    EXPECT_EQ(expected, query_helper->get_endpoint(input));
}
