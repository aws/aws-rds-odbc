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

#include <aws/core/Aws.h>
#include <aws/secretsmanager/SecretsManagerClient.h>
#include <aws/secretsmanager/model/GetSecretValueRequest.h>

#include "secrets_manager_helper.h"
#include "logger_wrapper.h"

using testing::Property;
using testing::Return;
using testing::StrEq;

namespace {
    const Aws::String TEST_SECRET_ID{"secret_ID"};
    const Aws::String TEST_REGION{"us-east-2"};
    const Aws::String TEST_USERNAME{"test-user"};
    const Aws::String TEST_PASSWORD{"test-password"};
    const auto TEST_SECRET_STRING = R"({"username": ")" + TEST_USERNAME + R"(", "password": ")" + TEST_PASSWORD + R"("})";
    const auto TEST_SECRET = Aws::Utils::Json::JsonValue(TEST_SECRET_STRING);
}

Aws::SDKOptions testSdkOptions;

class SecretsManagerHelperTest : public testing::Test {
  protected:
    std::shared_ptr<MOCK_SECRETS_MANAGER_CLIENT> mockSmClient;

    // Runs once per suite
    static void SetUpTestSuite() {
      Aws::InitAPI(testSdkOptions);
    }
    static void TearDownTestSuite() {
      Aws::ShutdownAPI(testSdkOptions);
    }

    // Runs per test case
    void SetUp() override {
      mockSmClient = std::make_shared<MOCK_SECRETS_MANAGER_CLIENT>();
    }
    void TearDown() override {}
};

TEST_F(SecretsManagerHelperTest, FetchCredentialsOK) {
  const Aws::SecretsManager::Model::GetSecretValueResult result = Aws::SecretsManager::Model::GetSecretValueResult().WithSecretString(TEST_SECRET_STRING);
  const Aws::SecretsManager::Model::GetSecretValueOutcome successfulOutcome = Aws::SecretsManager::Model::GetSecretValueOutcome(result);

  EXPECT_CALL(*mockSmClient, GetSecretValue(
    Property("GetSecretId", &Aws::SecretsManager::Model::GetSecretValueRequest::GetSecretId, StrEq(TEST_SECRET_ID))
  )).WillOnce(Return(successfulOutcome));

  SecretsManagerHelper smHelper(mockSmClient);

  bool res = smHelper.FetchCredentials(TEST_SECRET_ID);
  EXPECT_TRUE(res);
  EXPECT_EQ(smHelper.GetUsername(), TEST_USERNAME);
  EXPECT_EQ(smHelper.GetPassword(), TEST_PASSWORD);
}

TEST_F(SecretsManagerHelperTest, FetchCredentialsError) {
  const Aws::SecretsManager::Model::GetSecretValueOutcome erroredOutcome = Aws::SecretsManager::Model::GetSecretValueOutcome();

  EXPECT_CALL(*mockSmClient, GetSecretValue(
    Property("GetSecretId", &Aws::SecretsManager::Model::GetSecretValueRequest::GetSecretId, StrEq(TEST_SECRET_ID))
  )).WillOnce(Return(erroredOutcome));

  SecretsManagerHelper smHelper(mockSmClient);

  bool res = smHelper.FetchCredentials(TEST_SECRET_ID);
  EXPECT_FALSE(res);
}

TEST_F(SecretsManagerHelperTest, ParseRegionFromSecretId_FullArn_NoRegion) {
  Aws::String secretIdWithRegion = "arn:aws:secretsmanager:us-east-2:secretId";
  Aws::String region = "";

  SecretsManagerHelper::ParseRegionFromSecretId(secretIdWithRegion, region);
  EXPECT_EQ(region, "us-east-2");
}

TEST_F(SecretsManagerHelperTest, ParseRegionFromSecretId_FullArn_RegionInput) {
  Aws::String secretIdWithRegion = "arn:aws:secretsmanager:us-east-2:secretId";
  Aws::String region = "us-west-1";

  SecretsManagerHelper::ParseRegionFromSecretId(secretIdWithRegion, region);
  EXPECT_EQ(region, "us-east-2");
}

TEST_F(SecretsManagerHelperTest, ParseRegionFromSecretId_IdOnly_RegionInput) {
  Aws::String secretIdWithRegion = "secretId";
  Aws::String region = "us-west-1";

  SecretsManagerHelper::ParseRegionFromSecretId(secretIdWithRegion, region);
  EXPECT_EQ(region, "us-west-1");
}

TEST_F(SecretsManagerHelperTest, ParseRegionFromSecretId_IdOnly_Default) {
  Aws::String secretIdWithRegion = "secretId";
  Aws::String region = "";

  SecretsManagerHelper::ParseRegionFromSecretId(secretIdWithRegion, region);
  EXPECT_EQ(region, "us-east-1");
}
