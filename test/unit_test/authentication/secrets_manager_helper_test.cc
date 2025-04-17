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

#include <aws/core/Aws.h>
#include <aws/secretsmanager/SecretsManagerClient.h>
#include <aws/secretsmanager/model/GetSecretValueRequest.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "../mock_objects.h"
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
