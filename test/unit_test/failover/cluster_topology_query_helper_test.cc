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

#include "string_helper.h"
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

TEST_F(ClusterTopologyQueryHelperTest, CreateHost) {
    SQLTCHAR* node_id = AS_SQLTCHAR(TEXT("node_id"));
    int port = 1;
    bool is_writer = false;
    float cpu_usage = 2;
    float replica_lag = 3;
    float weight = std::round(replica_lag) * 100L + std::round(cpu_usage);

    std::shared_ptr<ClusterTopologyQueryHelper> query_helper =
        std::make_shared<ClusterTopologyQueryHelper>(port, "?", TEXT(""), TEXT(""), TEXT(""));

    HostInfo host = query_helper->CreateHost(node_id, false, cpu_usage, replica_lag);
    std::string expected = StringHelper::ToString(node_id);
    EXPECT_EQ(expected, host.GetHost());
    EXPECT_EQ(port, host.GetPort());
    EXPECT_EQ(weight, host.GetWeight());
    EXPECT_EQ(is_writer, host.IsHostWriter());
}

TEST_F(ClusterTopologyQueryHelperTest, GetEndpoint) {
    std::shared_ptr<ClusterTopologyQueryHelper> query_helper =
        std::make_shared<ClusterTopologyQueryHelper>(1234, "?", TEXT(""), TEXT(""), TEXT(""));
    SQLTCHAR* node_id = AS_SQLTCHAR(TEXT("node-id"));
    std::string expected = StringHelper::ToString(node_id);
    EXPECT_EQ(expected, query_helper->GetEndpoint(node_id));
}

TEST_F(ClusterTopologyQueryHelperTest, GetEndpoint_extra_values) {
    std::string endpoint_template = "?.other-values";
    std::shared_ptr<ClusterTopologyQueryHelper> query_helper =
        std::make_shared<ClusterTopologyQueryHelper>(1234, endpoint_template, TEXT(""), TEXT(""), TEXT(""));
    SQLTCHAR* node_id = AS_SQLTCHAR(TEXT("node-id"));
    std::string narrow_id = StringHelper::ToString(node_id);
    std::string expected = std::string(endpoint_template).replace(endpoint_template.find("?"), 1, narrow_id.c_str());
    EXPECT_EQ(expected, query_helper->GetEndpoint(node_id));
}
