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

#include "string_to_number_converter.h"

class StringToNumberConverterTest : public testing::Test {
protected:
    // Runs once per suite
    static void SetUpTestSuite() {}
    static void TearDownTestSuite() {}
    // Runs per test case
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(StringToNumberConverterTest, parse_str_to_int) {
    const char* number = "5432";
    int expected = 5432;
    int actual = StringToNumberConverter::Atoi(number);
    EXPECT_EQ(expected, actual);
}

TEST_F(StringToNumberConverterTest, parse_str_to_float) {
    const char* number = "3.1415";
    double expected = 3.1415;
    double actual = StringToNumberConverter::Atof(number);

    // Floating point comparison
    EXPECT_TRUE(fabs(expected-actual) < 0.00001);
}

TEST_F(StringToNumberConverterTest, parse_str_to_long) {
    const char* number = "4294967296";
    int64_t expected = 4294967296;
    int64_t actual = StringToNumberConverter::Atol(number);

    // Floating point comparison
    EXPECT_EQ(expected, actual);
}

TEST_F(StringToNumberConverterTest, parse_str_to_long_long) {
    const char* number = "9223372036854775807";
    int64_t expected = 9223372036854775807;
    int64_t actual = StringToNumberConverter::Atoll(number);
    EXPECT_EQ(expected, actual);
}

