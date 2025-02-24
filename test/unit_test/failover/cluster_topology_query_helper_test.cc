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

#include <gtest/gtest.h>

#include <algorithm>
#include <string>

#include <cluster_topology_query_helper.h>

#include "text_helper.h"
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
    SQLTCHAR* node_id = AS_SQLTCHAR(TEXT("node_id"));
    int port = 1;
    bool is_writer = false;
    float cpu_usage = 2;
    float replica_lag = 3;
    float weight = std::round(replica_lag) * 100L + std::round(cpu_usage);
    SQL_TIMESTAMP_STRUCT timestamp{2000, 01, 01, 01, 01, 01, 01};

    std::shared_ptr<ClusterTopologyQueryHelper> query_helper =
        std::make_shared<ClusterTopologyQueryHelper>(port, "?", "", "", "");

    HostInfo host = query_helper->create_host(node_id, false, cpu_usage, replica_lag, timestamp);
    std::string expected;
#ifdef UNICODE
    std::wstring node_id_wstr(node_id);
    expected = std::string(node_id_wstr.begin(), node_id_wstr.end());
#else
    expected = std::string(AS_CHAR(node_id));
#endif
    EXPECT_EQ(expected, host.get_host());
    EXPECT_EQ(port, host.get_port());
    EXPECT_EQ(weight, host.get_weight());
    EXPECT_EQ(is_writer, host.is_host_writer());
}

TEST_F(ClusterTopologyQueryHelperTest, get_endpoint) {
    std::shared_ptr<ClusterTopologyQueryHelper> query_helper =
        std::make_shared<ClusterTopologyQueryHelper>(1234, "?", "", "", "");
    SQLTCHAR* node_id = AS_SQLTCHAR(TEXT("node-id"));
    std::string expected;
#ifdef UNICODE
    std::wstring node_id_wstr(node_id);
    expected = std::string(node_id_wstr.begin(), node_id_wstr.end());
#else
    expected = std::string(AS_CHAR(node_id));
#endif
    EXPECT_EQ(expected, query_helper->get_endpoint(node_id));
}

TEST_F(ClusterTopologyQueryHelperTest, get_endpoint_extra_values) {
    std::string endpoint_template = "?.other-values";
    std::shared_ptr<ClusterTopologyQueryHelper> query_helper =
        std::make_shared<ClusterTopologyQueryHelper>(1234, endpoint_template, "", "", "");
    SQLTCHAR* node_id = AS_SQLTCHAR(TEXT("node-id"));
    std::string expected;
#ifdef UNICODE
    std::wstring node_id_wstr(node_id);
    std::string narrow_id = std::string(node_id_wstr.begin(), node_id_wstr.end());
    expected = std::string(endpoint_template).replace(endpoint_template.find("?"), 1, narrow_id.c_str());
#else
    expected = std::string(endpoint_template).replace(endpoint_template.find("?"), 1, AS_CHAR(node_id));
#endif
    EXPECT_EQ(expected, query_helper->get_endpoint(node_id));
}
