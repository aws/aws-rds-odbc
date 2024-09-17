# Modifications Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
#
# Copyright (c) 2007, 2024, Oracle and/or its affiliates.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 2.0,
# as published by the Free Software Foundation.
#
# This program is designed to work with certain software (including
# but not limited to OpenSSL) that is licensed under separate terms, as
# designated in a particular file or component or in included license
# documentation. The authors of MySQL hereby grant you an additional
# permission to link the program and your derivative works with the
# separately licensed software that they have either included with
# the program or referenced in the documentation.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See
# the GNU General Public License, version 2.0, for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
##########################################################################

project(integration_test)

cmake_minimum_required(VERSION 3.30 FATAL_ERROR)
set(CMAKE_CXX_STANDARD 20)

# External
## GoogleTest
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
)

FetchContent_MakeAvailable(googletest)

enable_testing()
include(GoogleTest)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

add_executable(
  # Executable Name
  integration_test

  # Runner / Utility 
  main.cc

  # Test Suites
)

include_directories("${CMAKE_SOURCE_DIR}/src/failover")

target_link_libraries(
  integration_test
  gtest_main
  gmock_main
  aws-rds-odbc
)

add_test(NAME integration_test COMMAND integration_test)
set_target_properties(integration_test PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
