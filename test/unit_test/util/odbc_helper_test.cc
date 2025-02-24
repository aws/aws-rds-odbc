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

