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

cmake_minimum_required(VERSION 3.22 FATAL_ERROR)
project("aws-rds-odbc")
set(CMAKE_CXX_STANDARD 20)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set(BUILD_SHARED_LIBS ON CACHE INTERNAL "Build as shared library")
else()
  set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "Build as static library")
endif()  

#-----------------------------------------------------

set(SRC
  src/host_info.cc

  src/authentication/adfs/adfs.cc
  src/authentication/authentication_provider.cc
  src/authentication/federation.cc
  src/authentication/html_util.cc
  src/authentication/okta/okta.cc
  src/authentication/secrets_manager_helper.cc

  src/failover/cluster_topology_info.cc

  src/host_availability/simple_host_availability_strategy.cc
  src/host_selector/random_host_selector.cc
  src/host_selector/round_robin_host_selector.cc

  src/limitless/limitless_query_helper.cc

  src/util/cache_map.cc
  src/util/logger_wrapper.cc
  src/util/odbc_helper.cc
)

set(INC
  src/host_info.h

  src/authentication/adfs/adfs.h
  src/authentication/authentication_provider.h
  src/authentication/federation.h
  src/authentication/html_util.h
  src/authentication/okta/okta.h
  src/authentication/secrets_manager_helper.h

  src/failover/cluster_topology_info.h

  src/host_availability/simple_host_availability_strategy.h
  src/host_selector/host_selector.h
  src/host_selector/random_host_selector.h
  src/host_selector/round_robin_host_selector.h

  src/limitless/limitless_query_helper.h

  src/util/cache_map.h
  src/util/logger_wrapper.h
  src/util/odbc_helper.h
)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  add_library(${PROJECT_NAME} SHARED ${SRC} ${INC})
else()
  add_library(${PROJECT_NAME} STATIC ${SRC} ${INC})
endif()
include_directories(${CMAKE_SOURCE_DIR})
include_directories("${CMAKE_SOURCE_DIR}/src")
include_directories("${CMAKE_SOURCE_DIR}/src/authentication")
include_directories("${CMAKE_SOURCE_DIR}/src/authentication/adfs")
include_directories("${CMAKE_SOURCE_DIR}/src/failover")
include_directories("${CMAKE_SOURCE_DIR}/src/host_availability")
include_directories("${CMAKE_SOURCE_DIR}/src/host_selector")
include_directories("${CMAKE_SOURCE_DIR}/src/util")

if(WIN32)
  add_compile_definitions(WIN32)
endif()

#-----------------------------------------------------

if(ENABLE_INTEGRATION_TESTS AND NOT ENABLE_UNIT_TESTS)
  add_subdirectory(test/integration_test)
elseif(ENABLE_UNIT_TESTS AND NOT ENABLE_INTEGRATION_TESTS)
  add_subdirectory(test/unit_test)
endif()

#-----------------------------------------------------
# macOS specific configuration

if(APPLE)
  #-----------------------------------------------------
  # ZLIB needs to be explicity required

  find_package(ZLIB REQUIRED)
endif()

#-----------------------------------------------------
# CTPL: Modern and efficient C++ Thread Pool Library

include(FetchContent)
FetchContent_Declare(
  ctpl
  URL https://github.com/vit-vit/CTPL/archive/refs/tags/ctpl_v.0.0.2.zip
)
FetchContent_MakeAvailable(ctpl)
include_directories(${ctpl_SOURCE_DIR})

#-----------------------------------------------------
# AWS SDK for C++

# Paths to search when attempting to find the AWS SDK
set(AWS_SDK_DIR "${CMAKE_SOURCE_DIR}/aws_sdk/install")
list(APPEND CMAKE_PREFIX_PATH ${AWS_SDK_DIR})
string(REPLACE ";" "/aws-cpp-sdk-all;" SYSTEM_MODULE_PATH "${CMAKE_SYSTEM_PREFIX_PATH}/aws-cpp-sdk-all")
list(APPEND CMAKE_PREFIX_PATH ${SYSTEM_MODULE_PATH})

find_package(AWSSDK REQUIRED COMPONENTS core rds secretsmanager sts)
include_directories(${AWSSDK_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} ${AWSSDK_LINK_LIBRARIES})

set(
  AWS_SDK_LIBS
  "aws-c-auth"
  "aws-c-cal"
  "aws-c-common"
  "aws-c-compression"
  "aws-c-event-stream"
  "aws-c-http"
  "aws-c-io"
  "aws-c-mqtt"
  "aws-c-s3"
  "aws-c-sdkutils"
  "aws-checksums"
  "aws-crt-cpp"
  "aws-cpp-sdk-core"
  "aws-cpp-sdk-sts"
  "aws-cpp-sdk-rds"
  "aws-cpp-sdk-secretsmanager"
)

set(AWS_SDK_LIBRARIES_EXPANDED "")
foreach(lib ${AWS_SDK_LIBS})
  if(APPLE OR CMAKE_SYSTEM_NAME STREQUAL "Linux")
    list(APPEND AWS_SDK_LIBRARIES_EXPANDED ${AWS_SDK_DIR}/lib/lib${lib}.a)
  endif()
  if(WIN32)
    list(APPEND AWS_SDK_LIBRARIES_EXPANDED ${AWS_SDK_DIR}/lib/${lib}.lib)
  endif()
endforeach()
message(STATUS "AWS_SDK_LIBRARIES_EXPANDED: ${AWS_SDK_LIBRARIES_EXPANDED}")

#-----------------------------------------------------
# Google logging library

set("BUILD_TESTING" OFF CACHE INTERNAL "Skip GLOG Tests")
FetchContent_Declare(
    glog
    GIT_REPOSITORY https://github.com/google/glog.git
    GIT_TAG        v0.7.1
)
FetchContent_MakeAvailable(glog)
include_directories(${glog_SOURCE_DIR})
target_link_libraries(${PROJECT_NAME} glog::glog)

#-----------------------------------------------------
# Find and add ODBC

if(APPLE)
  # Required to ensure unixodbc is found and used before iODBC
  set(ODBC_INCLUDE_DIR /opt/homebrew/opt/unixodbc/include)
endif()

find_package(ODBC REQUIRED)

message(STATUS "ODBC_INCLUDE_DIRS: ${ODBC_INCLUDE_DIRS}")
message(STATUS "ODBC_LIBRARIES: ${ODBC_LIBRARIES}")
include_directories(${ODBC_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} ${ODBC_LIBRARIES})

#-----------------------------------------------------
# Combine static libraries into the target static library

if(APPLE)
  add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "Combining static libraries into $<TARGET_FILE:${PROJECT_NAME}>"
    COMMAND libtool -static -o $<TARGET_FILE:${PROJECT_NAME}> $<TARGET_FILE:${PROJECT_NAME}> $<TARGET_FILE:glog::glog> ${AWS_SDK_LIBRARIES_EXPANDED}
  )
endif()

if(WIN32)
  add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND lib /OUT:$<TARGET_FILE:${PROJECT_NAME}> $<TARGET_FILE:${PROJECT_NAME}> $<TARGET_FILE:glog::glog> ${AWS_SDK_LIBRARIES_EXPANDED}
    COMMENT "Packaging additional .lib files into $<TARGET_FILE:${PROJECT_NAME}>"
)
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)
