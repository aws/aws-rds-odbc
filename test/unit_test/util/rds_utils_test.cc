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

#include "rds_utils.h"
#include "string_helper.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <string>

namespace {
    const std::string US_EAST_REGION_CLUSTER = "database-test-name.cluster-XYZ.us-east-2.rds.amazonaws.com";
    const std::string US_EAST_REGION_CLUSTER_READ_ONLY = "database-test-name.cluster-ro-XYZ.us-east-2.rds.amazonaws.com";
    const std::string US_EAST_REGION_INSTANCE = "instance-test-name.XYZ.us-east-2.rds.amazonaws.com";
    const std::string US_EAST_REGION_PROXY = "proxy-test-name.proxy-XYZ.us-east-2.rds.amazonaws.com";
    const std::string US_EAST_REGION_CUSTON_DOMAIN = "custom-test-name.cluster-custom-XYZ.us-east-2.rds.amazonaws.com";

    const std::string CHINA_REGION_CLUSTER = "database-test-name.cluster-XYZ.rds.cn-northwest-1.amazonaws.com.cn";
    const std::string CHINA_REGION_CLUSTER_READ_ONLY = "database-test-name.cluster-ro-XYZ.rds.cn-northwest-1.amazonaws.com.cn";
    const std::string CHINA_REGION_PROXY = "proxy-test-name.proxy-XYZ.rds.cn-northwest-1.amazonaws.com.cn";
    const std::string CHINA_REGION_CUSTON_DOMAIN = "custom-test-name.cluster-custom-XYZ.rds.cn-northwest-1.amazonaws.com.cn";
}  // namespace

class RdsUtilsTest : public testing::Test {
   protected:
    static void SetUpTestSuite() {}

    static void TearDownTestSuite() {}

    void SetUp() override {}

    void TearDown() override {}
};

TEST_F(RdsUtilsTest, is_rds_dns) {
    EXPECT_TRUE(RdsUtils::is_rds_dns(US_EAST_REGION_CLUSTER));
    EXPECT_TRUE(RdsUtils::is_rds_dns(US_EAST_REGION_CLUSTER_READ_ONLY));
    EXPECT_TRUE(RdsUtils::is_rds_dns(US_EAST_REGION_PROXY));
    EXPECT_TRUE(RdsUtils::is_rds_dns(US_EAST_REGION_CUSTON_DOMAIN));

    EXPECT_TRUE(RdsUtils::is_rds_dns(CHINA_REGION_CLUSTER));
    EXPECT_TRUE(RdsUtils::is_rds_dns(CHINA_REGION_CLUSTER_READ_ONLY));
    EXPECT_TRUE(RdsUtils::is_rds_dns(CHINA_REGION_PROXY));
    EXPECT_TRUE(RdsUtils::is_rds_dns(CHINA_REGION_CUSTON_DOMAIN));
}

TEST_F(RdsUtilsTest, is_rds_cluster_dns) {
    EXPECT_TRUE(RdsUtils::is_rds_cluster_dns(US_EAST_REGION_CLUSTER));
    EXPECT_TRUE(RdsUtils::is_rds_cluster_dns(US_EAST_REGION_CLUSTER_READ_ONLY));
    EXPECT_FALSE(RdsUtils::is_rds_cluster_dns(US_EAST_REGION_PROXY));
    EXPECT_FALSE(RdsUtils::is_rds_cluster_dns(US_EAST_REGION_CUSTON_DOMAIN));

    EXPECT_TRUE(RdsUtils::is_rds_cluster_dns(CHINA_REGION_CLUSTER));
    EXPECT_TRUE(RdsUtils::is_rds_cluster_dns(CHINA_REGION_CLUSTER_READ_ONLY));
    EXPECT_FALSE(RdsUtils::is_rds_cluster_dns(CHINA_REGION_PROXY));
    EXPECT_FALSE(RdsUtils::is_rds_cluster_dns(CHINA_REGION_CUSTON_DOMAIN));
}

TEST_F(RdsUtilsTest, is_rds_proxy_dns) {
    EXPECT_FALSE(RdsUtils::is_rds_proxy_dns(US_EAST_REGION_CLUSTER));
    EXPECT_FALSE(RdsUtils::is_rds_proxy_dns(US_EAST_REGION_CLUSTER_READ_ONLY));
    EXPECT_TRUE(RdsUtils::is_rds_proxy_dns(US_EAST_REGION_PROXY));
    EXPECT_FALSE(RdsUtils::is_rds_proxy_dns(US_EAST_REGION_CUSTON_DOMAIN));

    EXPECT_FALSE(RdsUtils::is_rds_proxy_dns(CHINA_REGION_CLUSTER));
    EXPECT_FALSE(RdsUtils::is_rds_proxy_dns(CHINA_REGION_CLUSTER_READ_ONLY));
    EXPECT_TRUE(RdsUtils::is_rds_proxy_dns(CHINA_REGION_PROXY));
    EXPECT_FALSE(RdsUtils::is_rds_proxy_dns(CHINA_REGION_CUSTON_DOMAIN));
}

TEST_F(RdsUtilsTest, is_rds_writer_cluster_dns) {
    EXPECT_TRUE(RdsUtils::is_rds_writer_cluster_dns(US_EAST_REGION_CLUSTER));
    EXPECT_FALSE(RdsUtils::is_rds_writer_cluster_dns(US_EAST_REGION_CLUSTER_READ_ONLY));
    EXPECT_FALSE(RdsUtils::is_rds_writer_cluster_dns(US_EAST_REGION_PROXY));
    EXPECT_FALSE(RdsUtils::is_rds_writer_cluster_dns(US_EAST_REGION_CUSTON_DOMAIN));

    EXPECT_TRUE(RdsUtils::is_rds_writer_cluster_dns(CHINA_REGION_CLUSTER));
    EXPECT_FALSE(RdsUtils::is_rds_writer_cluster_dns(CHINA_REGION_CLUSTER_READ_ONLY));
    EXPECT_FALSE(RdsUtils::is_rds_writer_cluster_dns(CHINA_REGION_PROXY));
    EXPECT_FALSE(RdsUtils::is_rds_writer_cluster_dns(CHINA_REGION_CUSTON_DOMAIN));
}

TEST_F(RdsUtilsTest, id_rds_custom_cluster) {
    EXPECT_FALSE(RdsUtils::is_rds_custom_cluster_dns(US_EAST_REGION_CLUSTER));
    EXPECT_FALSE(RdsUtils::is_rds_custom_cluster_dns(US_EAST_REGION_CLUSTER_READ_ONLY));
    EXPECT_FALSE(RdsUtils::is_rds_custom_cluster_dns(US_EAST_REGION_PROXY));
    EXPECT_TRUE(RdsUtils::is_rds_custom_cluster_dns(US_EAST_REGION_CUSTON_DOMAIN));

    EXPECT_FALSE(RdsUtils::is_rds_custom_cluster_dns(CHINA_REGION_CLUSTER));
    EXPECT_FALSE(RdsUtils::is_rds_custom_cluster_dns(CHINA_REGION_CLUSTER_READ_ONLY));
    EXPECT_FALSE(RdsUtils::is_rds_custom_cluster_dns(CHINA_REGION_PROXY));
    EXPECT_TRUE(RdsUtils::is_rds_custom_cluster_dns(CHINA_REGION_CUSTON_DOMAIN));
}

TEST_F(RdsUtilsTest, get_rds_instance_host_pattern) {
    const std::string expected_pattern = "?.XYZ.us-east-2.rds.amazonaws.com";
    EXPECT_EQ(expected_pattern, RdsUtils::get_rds_instance_host_pattern(US_EAST_REGION_CLUSTER));
    EXPECT_EQ(expected_pattern, RdsUtils::get_rds_instance_host_pattern(US_EAST_REGION_CLUSTER_READ_ONLY));
    EXPECT_EQ(expected_pattern, RdsUtils::get_rds_instance_host_pattern(US_EAST_REGION_PROXY));
    EXPECT_EQ(expected_pattern, RdsUtils::get_rds_instance_host_pattern(US_EAST_REGION_CUSTON_DOMAIN));

    const std::string expected_china_pattern = "?.XYZ.rds.cn-northwest-1.amazonaws.com.cn";
    EXPECT_EQ(expected_china_pattern, RdsUtils::get_rds_instance_host_pattern(CHINA_REGION_CLUSTER));
    EXPECT_EQ(expected_china_pattern, RdsUtils::get_rds_instance_host_pattern(CHINA_REGION_CLUSTER_READ_ONLY));
    EXPECT_EQ(expected_china_pattern, RdsUtils::get_rds_instance_host_pattern(CHINA_REGION_PROXY));
    EXPECT_EQ(expected_china_pattern, RdsUtils::get_rds_instance_host_pattern(CHINA_REGION_CUSTON_DOMAIN));
}

TEST_F(RdsUtilsTest, get_rds_cluster_host_url) {
    EXPECT_EQ(US_EAST_REGION_CLUSTER, RdsUtils::get_rds_cluster_host_url(US_EAST_REGION_CLUSTER));
    EXPECT_EQ(US_EAST_REGION_CLUSTER, RdsUtils::get_rds_cluster_host_url(US_EAST_REGION_CLUSTER_READ_ONLY));
    EXPECT_EQ(std::string(), RdsUtils::get_rds_cluster_host_url(US_EAST_REGION_PROXY));
    EXPECT_EQ(std::string(), RdsUtils::get_rds_cluster_host_url(US_EAST_REGION_CUSTON_DOMAIN));

    EXPECT_EQ(CHINA_REGION_CLUSTER, RdsUtils::get_rds_cluster_host_url(CHINA_REGION_CLUSTER));
    EXPECT_EQ(CHINA_REGION_CLUSTER, RdsUtils::get_rds_cluster_host_url(CHINA_REGION_CLUSTER_READ_ONLY));
    EXPECT_EQ(std::string(), RdsUtils::get_rds_cluster_host_url(CHINA_REGION_PROXY));
    EXPECT_EQ(std::string(), RdsUtils::get_rds_cluster_host_url(CHINA_REGION_CUSTON_DOMAIN));
}

TEST_F(RdsUtilsTest, get_rds_cluster_id) {
    EXPECT_EQ("database-test-name", RdsUtils::get_rds_cluster_id(US_EAST_REGION_CLUSTER));
    EXPECT_EQ("database-test-name", RdsUtils::get_rds_cluster_id(US_EAST_REGION_CLUSTER_READ_ONLY));
    EXPECT_EQ(std::string(), RdsUtils::get_rds_cluster_id(US_EAST_REGION_INSTANCE));

    EXPECT_EQ("proxy-test-name", RdsUtils::get_rds_cluster_id(US_EAST_REGION_PROXY));
    EXPECT_EQ("custom-test-name", RdsUtils::get_rds_cluster_id(US_EAST_REGION_CUSTON_DOMAIN));

    EXPECT_EQ("database-test-name", RdsUtils::get_rds_cluster_id(CHINA_REGION_CLUSTER));
    EXPECT_EQ("database-test-name", RdsUtils::get_rds_cluster_id(CHINA_REGION_CLUSTER_READ_ONLY));
    EXPECT_EQ("proxy-test-name", RdsUtils::get_rds_cluster_id(CHINA_REGION_PROXY));
    EXPECT_EQ("custom-test-name", RdsUtils::get_rds_cluster_id(CHINA_REGION_CUSTON_DOMAIN));
}

TEST_F(RdsUtilsTest, get_rds_instance_id) {
    EXPECT_EQ("instance-test-name", RdsUtils::get_rds_instance_id(US_EAST_REGION_INSTANCE));
    EXPECT_EQ(std::string(), RdsUtils::get_rds_instance_id(US_EAST_REGION_CLUSTER_READ_ONLY));
    EXPECT_EQ(std::string(), RdsUtils::get_rds_instance_id(US_EAST_REGION_CLUSTER));
}

#ifdef WIN32
TEST_F(RdsUtilsTest, sqlwchar_to_string_converted) {
    EXPECT_EQ("Wide character string", StringHelper::to_string(L"Wide character string"));
}
#endif
