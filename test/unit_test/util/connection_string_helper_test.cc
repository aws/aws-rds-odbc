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

TEST_F(ConnectionStringHelperTest, parse_connection_string_ascii) {
    std::map<std::string, std::string> dest_map;
    ConnectionStringHelper::ParseConnectionString("key1=value1;key2=value2;key3=value3;", dest_map);
    EXPECT_EQ(dest_map.size(), 3);
    EXPECT_EQ(dest_map["key1"], "value1");
    EXPECT_EQ(dest_map["key2"], "value2");
    EXPECT_EQ(dest_map["key3"], "value3");
}

TEST_F(ConnectionStringHelperTest, parse_connection_string_unicode) {
    std::map<std::wstring, std::wstring> dest_map;
    ConnectionStringHelper::ParseConnectionStringW(L"key1=value1;key2=value2;key3=value3;", dest_map);
    EXPECT_EQ(dest_map.size(), 3);
    EXPECT_EQ(dest_map[L"key1"], L"value1");
    EXPECT_EQ(dest_map[L"key2"], L"value2");
    EXPECT_EQ(dest_map[L"key3"], L"value3");
}

TEST_F(ConnectionStringHelperTest, parse_connection_string_no_trailing_semicolon) {
    std::map<std::string, std::string> dest_map;
    ConnectionStringHelper::ParseConnectionString("key1=value1;key2=value2;key3=value3", dest_map);
    EXPECT_EQ(dest_map.size(), 3);
    EXPECT_EQ(dest_map["key1"], "value1");
    EXPECT_EQ(dest_map["key2"], "value2");
    EXPECT_EQ(dest_map["key3"], "value3");
}

TEST_F(ConnectionStringHelperTest, parse_connection_string_nothing) {
    std::map<std::string, std::string> dest_map;
    ConnectionStringHelper::ParseConnectionString("", dest_map);
    EXPECT_EQ(dest_map.size(), 0);
}
