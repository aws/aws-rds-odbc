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

cmake_minimum_required(VERSION 3.30 FATAL_ERROR)
project("aws-rds-odbc")
set(CMAKE_CXX_STANDARD 20)
set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "Build as static libraries")

#-----------------------------------------------------

include_directories("${CMAKE_SOURCE_DIR}/src/failover")
include_directories("${CMAKE_SOURCE_DIR}/src/util")
set(SRC
  # Util
  src/util/logger_wrapper.cc

  # Failover
  src/failover/cluster_topology_info.cc
  src/failover/host_info.cc
)

set(INC
  # Util
  src/util/logger_wrapper.h

  # Failover
  src/failover/cluster_topology_info.h
  src/failover/host_info.h
)

add_library(${PROJECT_NAME} STATIC
  ${SRC} ${INC}
)

#-----------------------------------------------------

if(ENABLE_INTEGRATION_TESTS AND NOT ENABLE_UNIT_TESTS)
    add_subdirectory(test/integration_test)
elseif(ENABLE_UNIT_TESTS AND NOT ENABLE_INTEGRATION_TESTS)
    add_subdirectory(test/unit_test)
endif()

#-----------------------------------------------------

include(FetchContent)
FetchContent_Declare(
  ctpl
  URL https://github.com/vit-vit/CTPL/archive/refs/tags/ctpl_v.0.0.2.zip
)
FetchContent_MakeAvailable(ctpl)

#-----------------------------------------------------
# Path for script built AWS SDK

LIST(APPEND CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/aws_sdk/install")

# Path for system built AWS SDK
string(REPLACE ";" "/aws-cpp-sdk-all;" SYSTEM_MODULE_PATH "${CMAKE_SYSTEM_PREFIX_PATH}/aws-cpp-sdk-all")
list(APPEND CMAKE_PREFIX_PATH ${SYSTEM_MODULE_PATH})

set(SERVICE_COMPONENTS rds secretsmanager)
find_package(AWSSDK REQUIRED COMPONENTS ${SERVICE_COMPONENTS})
target_link_libraries(${PROJECT_NAME} ${AWSSDK_LINK_LIBRARIES})

#-----------------------------------------------------

set(LOG4CPLUS_SOURCE_DIR "${CMAKE_BINARY_DIR}/third_party/log4cplus")
FetchContent_Declare(
  log4cplus
  GIT_REPOSITORY https://github.com/log4cplus/log4cplus.git
)
FetchContent_MakeAvailable(log4cplus)
target_link_libraries(${PROJECT_NAME} log4cplus::log4cplus)

#-----------------------------------------------------

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR})
include_directories(${ctpl_SOURCE_DIR})
include_directories(${log4cplus_SOURCE_DIR})
