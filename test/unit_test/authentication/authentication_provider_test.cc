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
    char* token;

    // Runs once per suite
    static void SetUpTestSuite() {}
    static void TearDownTestSuite() {}

    // Runs per test case
    void SetUp() override {
        token = (char*) malloc(max_token_size * sizeof(char));
        strcpy(token, empty_token);
    }
    void TearDown() override {
        free(token);
    }
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

TEST_F(AuthenticationProviderTest, GenerateConnectAuthToken_EmptyAdfsConf) {
    const char* username = "GenerateConnectAuthToken_BadAdfsConf";
    Aws::Auth::AWSCredentials credentials;
    FederatedAuthConfig empty_conf{};
    bool result = GenerateConnectAuthToken(token, max_token_size, hostname, region, atol(port), username, ADFS, empty_conf);
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
    bool result = GenerateConnectAuthToken(token, max_token_size, hostname, region, atol(port), username, ADFS, missing_fields_conf);
    EXPECT_FALSE(result);
    EXPECT_STREQ("", credentials.GetAWSAccessKeyId().c_str());
    EXPECT_STREQ("", credentials.GetAWSSecretKey().c_str());
    EXPECT_STREQ("", credentials.GetSessionToken().c_str());
}

TEST_F(AuthenticationProviderTest, GenerateConnectAuthToken_EmptyOktaConf) {
    const char* username = "GenerateConnectAuthToken_EmptyOktaConf";
    Aws::Auth::AWSCredentials credentials;
    FederatedAuthConfig empty_conf{};
    bool result = GenerateConnectAuthToken(token, max_token_size, hostname, region, atol(port), username, OKTA, empty_conf);
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
    bool result = GenerateConnectAuthToken(token, max_token_size, hostname, region, atol(port), username, OKTA, missing_fields_conf);
    EXPECT_FALSE(result);
    EXPECT_STREQ("", credentials.GetAWSAccessKeyId().c_str());
    EXPECT_STREQ("", credentials.GetAWSSecretKey().c_str());
    EXPECT_STREQ("", credentials.GetSessionToken().c_str());
}
