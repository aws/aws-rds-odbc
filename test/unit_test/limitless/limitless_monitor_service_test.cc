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

#include "limitless_monitor_service.h"

#include <cstdio>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "../mock_objects.h"
#include "connection_string_helper.h"
#include "connection_string_keys.h"
#include "string_helper.h"

using testing::Property;
using testing::Return;
using testing::StrEq;
using testing::Invoke;

static const SQLTCHAR *test_connection_string_lazy_c_str;
static const SQLTCHAR *test_connection_string_immediate_c_str;
const static int test_host_port = 5432;
SQLSTR conn_str_lazy, conn_str_immediate;

class LimitlessMonitorServiceTest : public testing::Test {
  protected:
    // Runs once per suite
    static void SetUpTestSuite() {
        std::map<SQLSTR, SQLSTR> conn_str_map;
        conn_str_map[SERVER_HOST_KEY] = TEXT("limitless.shardgrp-1234.us-east-2.rds.amazonaws.com");
        conn_str_map[LIMITLESS_MONITOR_INTERVAL_MS_KEY] = StringHelper::ToSQLSTR(std::to_string(TEST_LIMITLESS_MONITOR_INTERVAL_MS));

        conn_str_map[LIMITLESS_MODE_KEY] = LIMITLESS_MODE_VALUE_LAZY;
        conn_str_lazy = ConnectionStringHelper::BuildConnectionString(conn_str_map);
        test_connection_string_lazy_c_str = AS_SQLTCHAR(conn_str_lazy.c_str());

        conn_str_map[LIMITLESS_MODE_KEY] = LIMITLESS_MODE_VALUE_IMMEDIATE;
        conn_str_immediate = ConnectionStringHelper::BuildConnectionString(conn_str_map);
        test_connection_string_immediate_c_str = AS_SQLTCHAR(conn_str_immediate.c_str());
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

    // ensure monitor service believes the round robin host is valid
    EXPECT_CALL(*mock_monitor, TestConnectionToHost(testing::_)).Times(1).WillOnce(Return(true));

    LimitlessMonitorService limitless_monitor_service;
    std::string test_service_id = "";

    limitless_monitor_service.NewService(test_service_id, test_connection_string_lazy_c_str, test_host_port, mock_monitor);
    EXPECT_EQ(test_service_id, "limitless");
    // mock_monitor is now nullptr, as limitless_monitor_service moves it

    bool service_exists = limitless_monitor_service.CheckService(test_service_id);
    EXPECT_TRUE(service_exists);

    std::this_thread::sleep_for(std::chrono::milliseconds(2 * TEST_LIMITLESS_MONITOR_INTERVAL_MS));
    std::shared_ptr<HostInfo> host_info = limitless_monitor_service.GetHostInfo(test_service_id);
    EXPECT_TRUE(host_info != nullptr);
    if (host_info != nullptr) {
        EXPECT_EQ(host_info->GetHost(), "test_host1");
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

    // ensure monitor service believes the round robin hosts are valid
    EXPECT_CALL(*mock_monitor1, TestConnectionToHost(testing::_)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*mock_monitor2, TestConnectionToHost(testing::_)).Times(1).WillOnce(Return(true));
    // as mock_monitor3 doesn't have an array of limitless routers by the time GetHostInfo is called, TestConnectionToHost doesn't get called

    LimitlessMonitorService limitless_monitor_service;
    std::string mock_monitor1_id = "";
    std::string mock_monitor2_id = "monitor2";
    std::string mock_monitor3_id = "monitor3";

    // spin up the monitors
    limitless_monitor_service.NewService(mock_monitor1_id, test_connection_string_lazy_c_str, test_host_port, mock_monitor1);
    EXPECT_EQ(mock_monitor1_id, "limitless"); // as mock_monitor1_id is "", it is overwritten
    limitless_monitor_service.NewService(mock_monitor2_id, test_connection_string_lazy_c_str, test_host_port, mock_monitor2);
    EXPECT_EQ(mock_monitor2_id, "monitor2"); // monitor2 and monitor3 shouldn't be overwritten, as they are provided
    limitless_monitor_service.NewService(mock_monitor3_id, test_connection_string_lazy_c_str, test_host_port, mock_monitor3);
    EXPECT_EQ(mock_monitor3_id, "monitor3");
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
        EXPECT_EQ(host_info->GetHost(), "correct1");
        host_info = nullptr; // free it
    }
    host_info = limitless_monitor_service.GetHostInfo(mock_monitor2_id);
    EXPECT_TRUE(host_info != nullptr);
    if (host_info != nullptr) {
        EXPECT_EQ(host_info->GetHost(), "correct2");
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

    // ensure monitor service believes the round robin host is valid
    EXPECT_CALL(*mock_monitor, TestConnectionToHost(testing::_)).Times(1).WillOnce(Return(true));

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
        EXPECT_EQ(host_info->GetHost(), "test_host1");
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

TEST_F(LimitlessMonitorServiceTest, SelectHighestWeightOnBadRoundRobinHost) {
    std::shared_ptr<MOCK_LIMITLESS_ROUTER_MONITOR> mock_monitor = std::make_shared<MOCK_LIMITLESS_ROUTER_MONITOR>();
    mock_monitor->test_limitless_routers.push_back(HostInfo("hosta", 5432, UP, true, nullptr, 100)); // round robin choice (alphabetical; a < z)
    mock_monitor->test_limitless_routers.push_back(HostInfo("hostz", 5432, UP, true, nullptr, 200)); // highest weight choice (200 > 5)

    EXPECT_CALL(*mock_monitor, Open(true, test_connection_string_immediate_c_str, test_host_port, TEST_LIMITLESS_MONITOR_INTERVAL_MS, testing::_, testing::_))
        .Times(1)
        .WillOnce(Invoke(mock_monitor.get(), &MOCK_LIMITLESS_ROUTER_MONITOR::MockOpen));

    // there should be a single call testing the connection to the round robin host (hosta), and the mock monitor should return false
    // NOTE: gtest displays a warning as the mocked method will return false by default anyways,
    //   but this expect call is useful to ensure the round robin host is chosen first
    EXPECT_CALL(*mock_monitor, TestConnectionToHost("hosta")).Times(1).WillOnce(Return(false));

    LimitlessMonitorService limitless_monitor_service;
    std::string test_service_id = "service_1";
    limitless_monitor_service.NewService(test_service_id, test_connection_string_immediate_c_str, test_host_port, mock_monitor);
    EXPECT_TRUE(limitless_monitor_service.CheckService(test_service_id));

    // check host immediately, as limitless mode is immediate
    std::shared_ptr<HostInfo> host_info = limitless_monitor_service.GetHostInfo(test_service_id);
    EXPECT_TRUE(host_info != nullptr);
    if (host_info != nullptr) {
        EXPECT_EQ(host_info->GetHost(), "hostz"); // highest weight host - not round robin choice
    }

    // clean up monitor service
    limitless_monitor_service.DecrementReferenceCounter(test_service_id);
    EXPECT_FALSE(limitless_monitor_service.CheckService(test_service_id));
}
