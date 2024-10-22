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

#include "mock_objects.h"

#include "authentication_provider.h"

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
    const char* username = "GenerateConnectAuthToken_BadAdfsConf";
    Aws::Auth::AWSCredentials credentials;
    // Removed username from conf
    FederatedAuthConfig missing_fields_conf{"endpoint", "1234", "relay", "role_arn", "idp_arn", "", "pass", "1234", "1234", "disable"};
    bool result = GenerateConnectAuthToken(token, max_token_size, hostname, region, atol(port), username, ADFS, missing_fields_conf);
    EXPECT_FALSE(result);
    EXPECT_STREQ("", credentials.GetAWSAccessKeyId().c_str());
    EXPECT_STREQ("", credentials.GetAWSSecretKey().c_str());
    EXPECT_STREQ("", credentials.GetSessionToken().c_str());
}
