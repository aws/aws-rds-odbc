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

#include <gtest/gtest.h>

#include "connection_string_helper.h"
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
