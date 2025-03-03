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

#include <cstdio>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "text_helper.h"
#include "../mock_objects.h"

#include "connection_string_helper.h"
#include "limitless_monitor_service.h"

using testing::Property;
using testing::Return;
using testing::StrEq;
using testing::Invoke;

static const SQLTCHAR *test_connection_string_lazy_c_str;
static const SQLTCHAR *test_connection_string_immediate_c_str;
const static int test_host_port = 5432;
#ifdef UNICODE
std::wstring conn_str_lazy, conn_str_immediate;
#else
std::string conn_str_lazy, conn_str_immediate;
#endif

class LimitlessMonitorServiceTest : public testing::Test {
  protected:
    // Runs once per suite
    static void SetUpTestSuite() {
        #ifdef UNICODE
        conn_str_lazy = L"LIMITLESSMODE=lazy;LIMITLESSMONITORINTERVALMS=" + std::to_wstring(TEST_LIMITLESS_MONITOR_INTERVAL_MS) + L";";
        test_connection_string_lazy_c_str = (SQLTCHAR *)conn_str_lazy.c_str();
        conn_str_immediate = L"LIMITLESSMODE=immediate;LIMITLESSMONITORINTERVALMS=" + std::to_wstring(TEST_LIMITLESS_MONITOR_INTERVAL_MS) + L";";
        test_connection_string_immediate_c_str = (SQLTCHAR *)conn_str_immediate.c_str();
        #else
        conn_str_lazy = "LIMITLESSMODE=lazy;LIMITLESSMONITORINTERVALMS=" + std::to_string(TEST_LIMITLESS_MONITOR_INTERVAL_MS) + ";";
        test_connection_string_lazy_c_str = (SQLTCHAR *)conn_str_lazy.c_str();
        conn_str_immediate = "LIMITLESSMODE=immediate;LIMITLESSMONITORINTERVALMS=" + std::to_string(TEST_LIMITLESS_MONITOR_INTERVAL_MS) + ";";
        test_connection_string_immediate_c_str = (SQLTCHAR *)conn_str_immediate.c_str();
        #endif
    }
    static void TearDownTestSuite() {}

    // Runs per test case
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(LimitlessMonitorServiceTest, SingleMonitorTest) {
    std::shared_ptr<MOCK_LIMITLESS_ROUTER_MONITOR> mock_monitor = std::make_shared<MOCK_LIMITLESS_ROUTER_MONITOR>();
    mock_monitor->test_limitless_routers.push_back(HostInfo("test_host1", 5432, UP, true, nullptr, 101)); // round robin should choose this one
    mock_monitor->test_limitless_routers.push_back(HostInfo("test_host2", 5432, UP, true, nullptr, 100));

    EXPECT_CALL(*mock_monitor, Open(false, test_connection_string_lazy_c_str, test_host_port, TEST_LIMITLESS_MONITOR_INTERVAL_MS, testing::_, testing::_))
        .Times(1)
        .WillOnce(Invoke(mock_monitor.get(), &MOCK_LIMITLESS_ROUTER_MONITOR::MockOpen));

    LimitlessMonitorService limitless_monitor_service;
    std::string test_service_id = "service_1";

    limitless_monitor_service.NewService(test_service_id, test_connection_string_lazy_c_str, test_host_port, mock_monitor);
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

    EXPECT_CALL(*mock_monitor1, Open(false, test_connection_string_lazy_c_str, test_host_port, TEST_LIMITLESS_MONITOR_INTERVAL_MS, testing::_, testing::_))
        .Times(1).WillOnce(Invoke(mock_monitor1.get(), &MOCK_LIMITLESS_ROUTER_MONITOR::MockOpen));
    EXPECT_CALL(*mock_monitor2, Open(false, test_connection_string_lazy_c_str, test_host_port, TEST_LIMITLESS_MONITOR_INTERVAL_MS, testing::_, testing::_))
        .Times(1).WillOnce(Invoke(mock_monitor2.get(), &MOCK_LIMITLESS_ROUTER_MONITOR::MockOpen));
    EXPECT_CALL(*mock_monitor3, Open(false, test_connection_string_lazy_c_str, test_host_port, TEST_LIMITLESS_MONITOR_INTERVAL_MS, testing::_, testing::_))
        .Times(1).WillOnce(Invoke(mock_monitor3.get(), &MOCK_LIMITLESS_ROUTER_MONITOR::MockOpen));

    LimitlessMonitorService limitless_monitor_service;
    std::string mock_monitor1_id = "monitor1";
    std::string mock_monitor2_id = "monitor2";
    std::string mock_monitor3_id = "monitor3";

    // spin up the monitors
    limitless_monitor_service.NewService(mock_monitor1_id, test_connection_string_lazy_c_str, test_host_port, mock_monitor1);
    limitless_monitor_service.NewService(mock_monitor2_id, test_connection_string_lazy_c_str, test_host_port, mock_monitor2);
    limitless_monitor_service.NewService(mock_monitor3_id, test_connection_string_lazy_c_str, test_host_port, mock_monitor3);
    // mock_monitor(1|2|3) are now nullptr, as limitless_monitor_service moves them

    // double check that this returns false, as monitor 1 already exists
    bool ret = limitless_monitor_service.NewService(mock_monitor1_id, test_connection_string_lazy_c_str, test_host_port, nullptr);
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

TEST_F(LimitlessMonitorServiceTest, ImmediateMonitorTest) {
    std::shared_ptr<MOCK_LIMITLESS_ROUTER_MONITOR> mock_monitor = std::make_shared<MOCK_LIMITLESS_ROUTER_MONITOR>();
    mock_monitor->test_limitless_routers.push_back(HostInfo("test_host1", 5432, UP, true, nullptr, 101)); // round robin should choose this one
    mock_monitor->test_limitless_routers.push_back(HostInfo("test_host2", 5432, UP, true, nullptr, 100));

    EXPECT_CALL(*mock_monitor, Open(true, test_connection_string_immediate_c_str, test_host_port, TEST_LIMITLESS_MONITOR_INTERVAL_MS, testing::_, testing::_))
        .Times(1)
        .WillOnce(Invoke(mock_monitor.get(), &MOCK_LIMITLESS_ROUTER_MONITOR::MockOpen));

    LimitlessMonitorService limitless_monitor_service;
    std::string test_service_id = "service_1";

    limitless_monitor_service.NewService(test_service_id, test_connection_string_immediate_c_str, test_host_port, mock_monitor);
    // mock_monitor is now nullptr, as limitless_monitor_service moves it

    bool service_exists = limitless_monitor_service.CheckService(test_service_id);
    EXPECT_TRUE(service_exists);

    // check host immediately
    std::shared_ptr<HostInfo> host_info = limitless_monitor_service.GetHostInfo(test_service_id);
    EXPECT_TRUE(host_info != nullptr);
    if (host_info != nullptr) {
        EXPECT_EQ(host_info->get_host(), "test_host1");
    }

    limitless_monitor_service.DecrementReferenceCounter(test_service_id);

    service_exists = limitless_monitor_service.CheckService(test_service_id);
    EXPECT_FALSE(service_exists);
}

TEST_F(LimitlessMonitorServiceTest, NoMonitorTest) {
    LimitlessMonitorService limitless_monitor_service;
    EXPECT_FALSE(limitless_monitor_service.CheckService("this_service_does_not_exist"));
    EXPECT_TRUE(limitless_monitor_service.GetHostInfo("this_one_neither") == nullptr);

    // these shouldn't cause exceptions or memory leaks, just do nothing -- verify with debugger or valgrind
    limitless_monitor_service.IncrementReferenceCounter("non_existent");
    limitless_monitor_service.DecrementReferenceCounter("non_existent");
}
