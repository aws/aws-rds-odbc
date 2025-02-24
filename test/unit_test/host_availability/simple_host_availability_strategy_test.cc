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
