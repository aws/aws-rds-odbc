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

#include "connection_string_helper.h"

#include <gtest/gtest.h>

#include "logger_wrapper.h"
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

TEST_F(ConnectionStringHelperTest, parse_connection_string_ansi) {
    std::map<std::string, std::string> dest_map;
    ConnectionStringHelper::ParseConnectionString("KEY1=value1;kEy2=value2;key3=value3;", dest_map);
    EXPECT_EQ(dest_map.size(), 3);
    EXPECT_EQ(dest_map["KEY1"], "value1");
    EXPECT_EQ(dest_map["KEY2"], "value2");
    EXPECT_EQ(dest_map["KEY3"], "value3");
}

TEST_F(ConnectionStringHelperTest, parse_connection_string_unicode) {
    std::map<std::wstring, std::wstring> dest_map;
    ConnectionStringHelper::ParseConnectionStringW(L"KEY1=value1;kEy2=value2;key3=value3;", dest_map);
    EXPECT_EQ(dest_map.size(), 3);
    EXPECT_EQ(dest_map[L"KEY1"], L"value1");
    EXPECT_EQ(dest_map[L"KEY2"], L"value2");
    EXPECT_EQ(dest_map[L"KEY3"], L"value3");
}

TEST_F(ConnectionStringHelperTest, parse_connection_string_no_trailing_semicolon) {
    std::map<std::string, std::string> dest_map;
    ConnectionStringHelper::ParseConnectionString("KEY1=value1;kEy2=value2;key3=value3", dest_map);
    EXPECT_EQ(dest_map.size(), 3);
    EXPECT_EQ(dest_map["KEY1"], "value1");
    EXPECT_EQ(dest_map["KEY2"], "value2");
    EXPECT_EQ(dest_map["KEY3"], "value3");
}

TEST_F(ConnectionStringHelperTest, parse_connection_string_nothing) {
    std::map<std::string, std::string> dest_map;
    ConnectionStringHelper::ParseConnectionString("", dest_map);
    EXPECT_EQ(dest_map.size(), 0);
}

TEST_F(ConnectionStringHelperTest, build_connection_string_ansi) {
    std::map<std::string, std::string> dest_map;
    dest_map["KEY"] = "value";
    std::string res = ConnectionStringHelper::BuildConnectionString(dest_map);
    EXPECT_EQ("KEY=value", res);
}

TEST_F(ConnectionStringHelperTest, build_connection_string_long_ansi) {
    std::map<std::string, std::string> dest_map;
    dest_map["A"] = "1";
    dest_map["B"] = "2";
    std::string res = ConnectionStringHelper::BuildConnectionString(dest_map);
    EXPECT_EQ("A=1;B=2", res);
}

TEST_F(ConnectionStringHelperTest, build_connection_string_unicode) {
    std::map<std::wstring, std::wstring> dest_map;
    dest_map[L"KEY"] = L"value";
    std::wstring res = ConnectionStringHelper::BuildConnectionStringW(dest_map);
    EXPECT_EQ(L"KEY=value", res);
}

TEST_F(ConnectionStringHelperTest, build_connection_string_long_unicode) {
    std::map<std::wstring, std::wstring> dest_map;
    dest_map[L"A"] = L"1";
    dest_map[L"B"] = L"2";
    std::wstring res = ConnectionStringHelper::BuildConnectionStringW(dest_map);
    EXPECT_EQ(L"A=1;B=2", res);
}
