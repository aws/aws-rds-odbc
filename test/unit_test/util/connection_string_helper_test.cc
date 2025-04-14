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

#include "connection_string_helper.h"
#include "logger_wrapper.h"
#include "string_helper.h"
#include "text_helper.h"

class ConnectionStringHelperTest : public testing::Test {
protected:
    // Runs once per suite
    static void SetUpTestSuite() {}
    static void TearDownTestSuite() {}
    // Runs per test case
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(ConnectionStringHelperTest, parse_connection_string) {
    std::map<MyStr, MyStr> dest_map;
    ConnectionStringHelper::ParseConnectionString(TEXT("KEY1=value1;kEy2=value2;key3=value3;"), dest_map);
    EXPECT_EQ(dest_map.size(), 3);
    EXPECT_EQ(dest_map[TEXT("KEY1")], TEXT("value1"));
    EXPECT_EQ(dest_map[TEXT("KEY2")], TEXT("value2"));
    EXPECT_EQ(dest_map[TEXT("KEY3")], TEXT("value3"));
}

TEST_F(ConnectionStringHelperTest, parse_connection_string_no_trailing_semicolon) {
    std::map<MyStr, MyStr> dest_map;
    ConnectionStringHelper::ParseConnectionString(TEXT("KEY1=value1;kEy2=value2;key3=value3"), dest_map);
    EXPECT_EQ(dest_map.size(), 3);
    EXPECT_EQ(dest_map[TEXT("KEY1")], TEXT("value1"));
    EXPECT_EQ(dest_map[TEXT("KEY2")], TEXT("value2"));
    EXPECT_EQ(dest_map[TEXT("KEY3")], TEXT("value3"));
}

TEST_F(ConnectionStringHelperTest, parse_connection_string_nothing) {
    std::map<MyStr, MyStr> dest_map;
    ConnectionStringHelper::ParseConnectionString(TEXT(""), dest_map);
    EXPECT_EQ(dest_map.size(), 0);
}

TEST_F(ConnectionStringHelperTest, build_connection_string) {
    std::map<MyStr, MyStr> dest_map;
    dest_map[TEXT("KEY")] = TEXT("value");
    MyStr res = ConnectionStringHelper::BuildConnectionString(dest_map);
    EXPECT_EQ(TEXT("KEY=value"), res);
}

TEST_F(ConnectionStringHelperTest, build_connection_string_long) {
    std::map<MyStr, MyStr> dest_map;
    dest_map[TEXT("A")] = TEXT("1");
    dest_map[TEXT("B")] = TEXT("2");
    MyStr res = ConnectionStringHelper::BuildConnectionString(dest_map);
    EXPECT_EQ(TEXT("A=1;B=2"), res);
}
