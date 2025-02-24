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

#include <simple_host_availability_strategy.h>

class SimpleHostAvailabilityStrategyTest : public testing::Test {
  protected:
    // Runs once per suite
    static void SetUpTestSuite() {}
    static void TearDownTestSuite() {}
    // Runs per test case
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(SimpleHostAvailabilityStrategyTest, set_host_availability) {
    std::shared_ptr<SimpleHostAvailabilityStrategy> simple_host_availability_strategy = 
        std::make_shared<SimpleHostAvailabilityStrategy>();

    simple_host_availability_strategy->set_host_availability(AVAILABLE);
    // set_host_availability does nothing so ensure it can be called
    // without throwing an exception
    SUCCEED();
}

TEST_F(SimpleHostAvailabilityStrategyTest, get_host_availability) {
    std::shared_ptr<SimpleHostAvailabilityStrategy> simple_host_availability_strategy = 
        std::make_shared<SimpleHostAvailabilityStrategy>();
    EXPECT_EQ(AVAILABLE, simple_host_availability_strategy->get_host_availability(AVAILABLE));
    EXPECT_EQ(NOT_AVAILABLE, simple_host_availability_strategy->get_host_availability(NOT_AVAILABLE));
}
