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

#include "string_to_number_converter.h"
#include "../mock_objects.h"

#include <authentication_provider.h>

namespace {
  const char* hostname = "hostName";
  const char* region = "us-east-1";
  const char* port = "1234";
  const char* cached_token = "pwd";
  const char* empty_token = "";
  const int max_token_size = 8;
}

class AuthenticationProviderTest : public testing::Test {
protected:
    char token[max_token_size] = "";

    // Runs once per suite
    static void SetUpTestSuite() {}
    static void TearDownTestSuite() {}
};

TEST_F(AuthenticationProviderTest, GetFedAuthTypeEnum_Test) {
    EXPECT_EQ(ADFS, GetFedAuthTypeEnum("adfs"));
    EXPECT_EQ(ADFS, GetFedAuthTypeEnum("ADFS"));
    EXPECT_EQ(IAM, GetFedAuthTypeEnum("iam"));
    EXPECT_EQ(IAM, GetFedAuthTypeEnum("IAM"));
    EXPECT_EQ(OKTA, GetFedAuthTypeEnum("okta"));
    EXPECT_EQ(OKTA, GetFedAuthTypeEnum("OKTA"));
    EXPECT_EQ(INVALID, GetFedAuthTypeEnum("garbage"));
    EXPECT_EQ(INVALID, GetFedAuthTypeEnum("unknown"));
}

TEST_F(AuthenticationProviderTest, GetCachedToken_Hit) {
    const char* username = "GetCachedToken_Hit";
    UpdateCachedToken(hostname, region, port, username, cached_token, "10");
    bool result = GetCachedToken(token, max_token_size, hostname, region, port, username);
    EXPECT_TRUE(result);
    ASSERT_EQ(strlen(cached_token), strlen(token));
    for (int i = 0; i < strlen(cached_token); ++i) {
        EXPECT_EQ(cached_token[i], token[i]);
    }
}

TEST_F(AuthenticationProviderTest, GetCachedToken_Miss) {
    const char* username = "GetCachedToken_Miss";
    bool result = GetCachedToken(token, max_token_size, hostname, region, port, username);
    EXPECT_FALSE(result);
    ASSERT_EQ(strlen(empty_token), strlen(token));
}

TEST_F(AuthenticationProviderTest, GetCachedToken_Expired) {
    const char* username = "GetCachedToken_Expire";
    UpdateCachedToken(hostname, region, port, username, cached_token, "-1");
    bool result = GetCachedToken(token, max_token_size, hostname, region, port, username);
    EXPECT_FALSE(result);
    ASSERT_EQ(strlen(empty_token), strlen(token));
}

TEST_F(AuthenticationProviderTest, GetCachedToken_TooLarge) {
    const char* username = "GetCachedToken_TooLarge";
    UpdateCachedToken(hostname, region, port, username, "oversized_password", "10");
    bool result = GetCachedToken(token, max_token_size - 1, hostname, region, port, username);
    EXPECT_FALSE(result);
    ASSERT_EQ(strlen(empty_token), strlen(token));
}

TEST_F(AuthenticationProviderTest, GetCachedToken_DifferentUsers) {
    char token1[max_token_size] = "";
    char token2[max_token_size] = "";
    const char* username1 = "user1";
    const char* username2 = "user2";
    UpdateCachedToken(hostname, region, port, username1, "1", "10");
    UpdateCachedToken(hostname, region, port, username2, "2", "10");
    bool user1_result = GetCachedToken(token1, max_token_size, hostname, region, port, username1);
    bool user2_result = GetCachedToken(token2, max_token_size, hostname, region, port, username2);
    EXPECT_TRUE(user1_result);
    EXPECT_TRUE(user2_result);
    ASSERT_EQ(strlen(token1), strlen(token2));
    EXPECT_TRUE(token1[0] != token2[0]);
}

TEST_F(AuthenticationProviderTest, GenerateConnectAuthToken_EmptyAdfsConf) {
    const char* username = "GenerateConnectAuthToken_BadAdfsConf";
    Aws::Auth::AWSCredentials credentials;
    FederatedAuthConfig empty_conf{};
    bool result = GenerateConnectAuthToken(token, max_token_size, hostname, region, safe_atol(port), username, ADFS, empty_conf);
    EXPECT_FALSE(result);
    EXPECT_STREQ("", credentials.GetAWSAccessKeyId().c_str());
    EXPECT_STREQ("", credentials.GetAWSSecretKey().c_str());
    EXPECT_STREQ("", credentials.GetSessionToken().c_str());
}

TEST_F(AuthenticationProviderTest, GenerateConnectAuthToken_MissingFieldsAdfsConf) {
    const char* username = "GenerateConnectAuthToken_MissingFieldsAdfsConf";
    Aws::Auth::AWSCredentials credentials;
    // Removed username from conf
    FederatedAuthConfig missing_fields_conf{"endpoint", "1234", "relay", "app_id", "role_arn", "idp_arn", "", "pass", "1234", "1234", "disable"};
    bool result = GenerateConnectAuthToken(token, max_token_size, hostname, region, safe_atol(port), username, ADFS, missing_fields_conf);
    EXPECT_FALSE(result);
    EXPECT_STREQ("", credentials.GetAWSAccessKeyId().c_str());
    EXPECT_STREQ("", credentials.GetAWSSecretKey().c_str());
    EXPECT_STREQ("", credentials.GetSessionToken().c_str());
}

TEST_F(AuthenticationProviderTest, GenerateConnectAuthToken_EmptyOktaConf) {
    const char* username = "GenerateConnectAuthToken_EmptyOktaConf";
    Aws::Auth::AWSCredentials credentials;
    FederatedAuthConfig empty_conf{};
    bool result = GenerateConnectAuthToken(token, max_token_size, hostname, region, safe_atol(port), username, OKTA, empty_conf);
    EXPECT_FALSE(result);
    EXPECT_STREQ("", credentials.GetAWSAccessKeyId().c_str());
    EXPECT_STREQ("", credentials.GetAWSSecretKey().c_str());
    EXPECT_STREQ("", credentials.GetSessionToken().c_str());
}

TEST_F(AuthenticationProviderTest, GenerateConnectAuthToken_MissingFieldsOktaConf) {
    const char* username = "GenerateConnectAuthToken_MissingFieldsOktaConf";
    Aws::Auth::AWSCredentials credentials;
    // Removed username from conf
    FederatedAuthConfig missing_fields_conf{"endpoint", "1234", "relay", "app_id", "role_arn", "idp_arn", "", "pass", "1234", "1234", "disable"};
    bool result = GenerateConnectAuthToken(token, max_token_size, hostname, region, safe_atol(port), username, OKTA, missing_fields_conf);
    EXPECT_FALSE(result);
    EXPECT_STREQ("", credentials.GetAWSAccessKeyId().c_str());
    EXPECT_STREQ("", credentials.GetAWSSecretKey().c_str());
    EXPECT_STREQ("", credentials.GetSessionToken().c_str());
}
