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
#include <odbc_helper.h>

#include "../mock_objects.h"

class OdbcHelperTest : public testing::Test {
  protected:
    // Runs once per suite
    static void SetUpTestSuite() {}
    static void TearDownTestSuite() {}
    // Runs per test case
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(OdbcHelperTest, CheckResult_SqlSucceeded) {
    void* mock_handle = reinterpret_cast<void*>(0x1234);

    EXPECT_TRUE(OdbcHelper::CheckResult(SQL_SUCCESS, "", mock_handle, SQL_HANDLE_STMT));
}

TEST_F(OdbcHelperTest, CheckResult_SqlError) {
    void* mock_handle = reinterpret_cast<void*>(0x1234);

    EXPECT_FALSE(OdbcHelper::CheckResult(SQL_ERROR, "", mock_handle, SQL_HANDLE_STMT));
}

