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

#include "host_info.h"

namespace {
  const std::string base_host_string = "hostName";
  const char* base_host_char = "hostName";
  const int base_port = 1234;
  const std::string base_host_port_pair = base_host_string + ":" + std::to_string(base_port);
}

class HostInfoTest : public testing::Test {
  protected:
    // Runs once per suite
    static void SetUpTestSuite() {}
    static void TearDownTestSuite() {}
    // Runs per test case
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(HostInfoTest, getPort) {
  HOST_INFO* hf_string = new HOST_INFO(base_host_string, base_port);
  EXPECT_EQ(base_port, hf_string->get_port());
  delete hf_string;
}

TEST_F(HostInfoTest, getHost) {
  HOST_INFO* hf_string = new HOST_INFO(base_host_string, base_port);
  EXPECT_EQ(base_host_string, hf_string->get_host());
  delete hf_string;
}

TEST_F(HostInfoTest, getHostPortPair) {
  HOST_INFO* hf_string = new HOST_INFO(base_host_string, base_port);
  EXPECT_EQ(base_host_port_pair, hf_string->get_host_port_pair());
  delete hf_string;
}
