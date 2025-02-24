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
