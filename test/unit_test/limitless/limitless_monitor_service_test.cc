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

#include "../mock_objects.h"

#include "limitless_monitor_service.h"

using testing::Property;
using testing::Return;
using testing::StrEq;

const static char *test_connection_string_c_str = "test_connection_string";
static int test_host_port = 5432;

class LimitlessMonitorServiceTest : public testing::Test {
  protected:
    // Runs once per suite
    static void SetUpTestSuite() {}
    static void TearDownTestSuite() {}

    // Runs per test case
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(LimitlessMonitorServiceTest, NewServiceTest) {
    std::shared_ptr<MOCK_LIMITLESS_ROUTER_MONITOR> mock_monitor = std::make_shared<MOCK_LIMITLESS_ROUTER_MONITOR>();

    EXPECT_CALL(*mock_monitor, Open(test_connection_string_c_str, test_host_port, DEFAULT_LIMITLESS_MONITOR_INTERVAL, testing::_, testing::_))
        .Times(1)
        .WillOnce([]() { /* do nothing; no ODBC calls */ });

    LimitlessMonitorService limitless_monitor_service;
    std::string test_service_id = "service_1";

    limitless_monitor_service.NewService(test_service_id, test_connection_string_c_str, test_host_port, mock_monitor);
    mock_monitor = nullptr;
    limitless_monitor_service.IncrementReferenceCounter(test_service_id);
    
    bool service_exists = limitless_monitor_service.CheckService(test_service_id);
    EXPECT_TRUE(service_exists);

    limitless_monitor_service.DecrementReferenceCounter(test_service_id);

    service_exists = limitless_monitor_service.CheckService(test_service_id);
    EXPECT_FALSE(service_exists);
}
