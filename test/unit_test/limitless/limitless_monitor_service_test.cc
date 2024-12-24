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

#include <cstdio>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "text_helper.h"
#include "../mock_objects.h"

#include "limitless_monitor_service.h"

using testing::Property;
using testing::Return;
using testing::StrEq;
using testing::Invoke;

static const SQLTCHAR *test_connection_string_c_str = (SQLTCHAR *)TEXT("test_connection_string");
const static int test_host_port = 5432;

class LimitlessMonitorServiceTest : public testing::Test {
  protected:
    // Runs once per suite
    static void SetUpTestSuite() {}
    static void TearDownTestSuite() {}

    // Runs per test case
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(LimitlessMonitorServiceTest, SingleMonitorTest) {
    std::shared_ptr<MOCK_LIMITLESS_ROUTER_MONITOR> mock_monitor = std::make_shared<MOCK_LIMITLESS_ROUTER_MONITOR>();
    mock_monitor->test_limitless_routers.push_back(HostInfo("test_host1", 5432, UP, true, nullptr, 101)); // round robin should choose this one
    mock_monitor->test_limitless_routers.push_back(HostInfo("test_host2", 5432, UP, true, nullptr, 100));

    EXPECT_CALL(*mock_monitor, Open(test_connection_string_c_str, test_host_port, DEFAULT_LIMITLESS_MONITOR_INTERVAL_MS, testing::_, testing::_))
        .Times(1)
        .WillOnce(Invoke(mock_monitor.get(), &MOCK_LIMITLESS_ROUTER_MONITOR::MockOpen));

    LimitlessMonitorService limitless_monitor_service;
    std::string test_service_id = "service_1";

    limitless_monitor_service.NewService(test_service_id, test_connection_string_c_str, test_host_port, mock_monitor);
    // mock_monitor is now nullptr, as limitless_monitor_service moves it

    bool service_exists = limitless_monitor_service.CheckService(test_service_id);
    EXPECT_TRUE(service_exists);

    std::this_thread::sleep_for(std::chrono::milliseconds(2 * TEST_LIMITLESS_MONITOR_INTERVAL_MS));
    std::shared_ptr<HostInfo> host_info = limitless_monitor_service.GetHostInfo(test_service_id);
    EXPECT_TRUE(host_info != nullptr);
    if (host_info != nullptr) {
        EXPECT_EQ(host_info->get_host(), "test_host1");
    }

    limitless_monitor_service.DecrementReferenceCounter(test_service_id);

    service_exists = limitless_monitor_service.CheckService(test_service_id);
    EXPECT_FALSE(service_exists);
}

TEST_F(LimitlessMonitorServiceTest, MultipleMonitorTest) {
    std::shared_ptr<MOCK_LIMITLESS_ROUTER_MONITOR> mock_monitor1 = std::make_shared<MOCK_LIMITLESS_ROUTER_MONITOR>();
    mock_monitor1->test_limitless_routers.push_back(HostInfo("correct1", 5432, UP, true, nullptr, 100));
    std::shared_ptr<MOCK_LIMITLESS_ROUTER_MONITOR> mock_monitor2 = std::make_shared<MOCK_LIMITLESS_ROUTER_MONITOR>();
    mock_monitor2->test_limitless_routers.push_back(HostInfo("incorrect1", 5432, UP, true, nullptr, 10));
    mock_monitor2->test_limitless_routers.push_back(HostInfo("incorrect2", 5432, UP, true, nullptr, 80));
    mock_monitor2->test_limitless_routers.push_back(HostInfo("correct2", 5432, UP, true, nullptr, 100));
    std::shared_ptr<MOCK_LIMITLESS_ROUTER_MONITOR> mock_monitor3 = std::make_shared<MOCK_LIMITLESS_ROUTER_MONITOR>();
    // mock_monitor3 will have an empty set of limitless routers -- couldn't fetch in time, essentially

    EXPECT_CALL(*mock_monitor1, Open(test_connection_string_c_str, test_host_port, DEFAULT_LIMITLESS_MONITOR_INTERVAL_MS, testing::_, testing::_))
        .Times(1).WillOnce(Invoke(mock_monitor1.get(), &MOCK_LIMITLESS_ROUTER_MONITOR::MockOpen));
    EXPECT_CALL(*mock_monitor2, Open(test_connection_string_c_str, test_host_port, DEFAULT_LIMITLESS_MONITOR_INTERVAL_MS, testing::_, testing::_))
        .Times(1).WillOnce(Invoke(mock_monitor2.get(), &MOCK_LIMITLESS_ROUTER_MONITOR::MockOpen));
    EXPECT_CALL(*mock_monitor3, Open(test_connection_string_c_str, test_host_port, DEFAULT_LIMITLESS_MONITOR_INTERVAL_MS, testing::_, testing::_))
        .Times(1).WillOnce(Invoke(mock_monitor3.get(), &MOCK_LIMITLESS_ROUTER_MONITOR::MockOpen));

    LimitlessMonitorService limitless_monitor_service;
    std::string mock_monitor1_id = "monitor1";
    std::string mock_monitor2_id = "monitor2";
    std::string mock_monitor3_id = "monitor3";

    // spin up the monitors
    limitless_monitor_service.NewService(mock_monitor1_id, test_connection_string_c_str, test_host_port, mock_monitor1);
    limitless_monitor_service.NewService(mock_monitor2_id, test_connection_string_c_str, test_host_port, mock_monitor2);
    limitless_monitor_service.NewService(mock_monitor3_id, test_connection_string_c_str, test_host_port, mock_monitor3);
    // mock_monitor(1|2|3) are now nullptr, as limitless_monitor_service moves them

    bool ret = limitless_monitor_service.NewService(mock_monitor1_id, test_connection_string_c_str, test_host_port, nullptr);
    EXPECT_FALSE(ret);

    // monitors should exist
    EXPECT_TRUE(limitless_monitor_service.CheckService(mock_monitor1_id));
    EXPECT_TRUE(limitless_monitor_service.CheckService(mock_monitor2_id));
    EXPECT_TRUE(limitless_monitor_service.CheckService(mock_monitor3_id));

    std::this_thread::sleep_for(std::chrono::milliseconds(2 * TEST_LIMITLESS_MONITOR_INTERVAL_MS));

    // get host info from each monitor
    std::shared_ptr<HostInfo> host_info = limitless_monitor_service.GetHostInfo(mock_monitor1_id);
    EXPECT_TRUE(host_info != nullptr);
    if (host_info != nullptr) {
        EXPECT_EQ(host_info->get_host(), "correct1");
        host_info = nullptr; // free it
    }
    host_info = limitless_monitor_service.GetHostInfo(mock_monitor2_id);
    EXPECT_TRUE(host_info != nullptr);
    if (host_info != nullptr) {
        EXPECT_EQ(host_info->get_host(), "correct2");
        host_info = nullptr; // free it
    }
    host_info = limitless_monitor_service.GetHostInfo(mock_monitor3_id);
    EXPECT_TRUE(host_info == nullptr);

    // simple decrements
    limitless_monitor_service.DecrementReferenceCounter(mock_monitor1_id);
    limitless_monitor_service.DecrementReferenceCounter(mock_monitor2_id);

    // several decrements (note that NewService acts as a single increment)
    limitless_monitor_service.IncrementReferenceCounter(mock_monitor3_id);
    limitless_monitor_service.IncrementReferenceCounter(mock_monitor3_id);
    limitless_monitor_service.DecrementReferenceCounter(mock_monitor3_id);
    limitless_monitor_service.DecrementReferenceCounter(mock_monitor3_id);
    limitless_monitor_service.DecrementReferenceCounter(mock_monitor3_id);

    // all shouldn't exist anymore
    EXPECT_FALSE(limitless_monitor_service.CheckService(mock_monitor1_id));
    EXPECT_FALSE(limitless_monitor_service.CheckService(mock_monitor2_id));
    EXPECT_FALSE(limitless_monitor_service.CheckService(mock_monitor3_id));
}

TEST_F(LimitlessMonitorServiceTest, NoMonitorTest) {
    LimitlessMonitorService limitless_monitor_service;
    EXPECT_FALSE(limitless_monitor_service.CheckService("this_service_does_not_exist"));
    EXPECT_TRUE(limitless_monitor_service.GetHostInfo("this_one_neither") == nullptr);

    // these shouldn't cause exceptions or memory leaks, just do nothing -- verify with debugger or valgrind
    limitless_monitor_service.IncrementReferenceCounter("non_existent");
    limitless_monitor_service.DecrementReferenceCounter("non_existent");
}
