# Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
#     http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# 
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
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
# Build type

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build (Debug or Release)" FORCE)
endif()

# Print the current build type
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

#-----------------------------------------------------

# Create an option for selecting the Unicode build type
option(UNICODE_BUILD "Enable Unicode build" OFF)

# Set output directory suffix based on the build type
if (UNICODE_BUILD)
  set(BUILD_DIR_SUFFIX "unicode")
  set(LIBRARY_NAME "${PROJECT_NAME}-w")
  add_definitions(-DUNICODE)
else()
  set(BUILD_DIR_SUFFIX "ansi")
  set(LIBRARY_NAME "${PROJECT_NAME}-a")
endif()

# Output a message indicating the build type
message(STATUS "Building ${BUILD_DIR_SUFFIX} version of ${PROJECT_NAME}")

#-----------------------------------------------------

set(SRC
  src/host_info.cc

  src/authentication/adfs/adfs.cc
  src/authentication/authentication_provider.cc
  src/authentication/federation.cc
  src/authentication/html_util.cc
  src/authentication/okta/okta.cc
  src/authentication/secrets_manager_helper.cc

  src/failover/cluster_topology_monitor.cc
  src/failover/cluster_topology_query_helper.cc
  src/failover/failover_service.cc

  src/host_availability/simple_host_availability_strategy.cc
  src/host_selector/highest_weight_host_selector.cc
  src/host_selector/random_host_selector.cc
  src/host_selector/round_robin_host_selector.cc

  src/limitless/limitless_monitor_service.cc
  src/limitless/limitless_query_helper.cc
  src/limitless/limitless_router_monitor.cc

  src/util/cluster_topology_helper.cc
  src/util/connection_string_helper.cc
  src/util/sliding_cache_map.cc
  src/util/logger_wrapper.cc
  src/util/rds_logger_service.cc
  src/util/rds_utils.cc
  src/util/odbc_helper.cc
  src/util/string_to_number_converter.cc
)

set(INC
  src/host_info.h

  src/authentication/adfs/adfs.h
  src/authentication/authentication_provider.h
  src/authentication/federation.h
  src/authentication/html_util.h
  src/authentication/okta/okta.h
  src/authentication/secrets_manager_helper.h

  src/dialect/dialect_aurora_postgres.h
  src/dialect/dialect.h

  src/failover/cluster_topology_monitor.h
  src/failover/cluster_topology_query_helper.h
  src/failover/failover_service.h

  src/host_availability/simple_host_availability_strategy.h
  src/host_selector/highest_weight_host_selector.h
  src/host_selector/host_selector.h
  src/host_selector/random_host_selector.h
  src/host_selector/round_robin_host_selector.h

  src/limitless/limitless_monitor_service.h
  src/limitless/limitless_query_helper.h
  src/limitless/limitless_router_monitor.h

  src/util/cluster_topology_helper.h
  src/util/connection_string_helper.h
  src/util/sliding_cache_map.h
  src/util/logger_wrapper.h
  src/util/rds_logger_service.h
  src/util/rds_utils.h
  src/util/odbc_helper.h
  src/util/string_helper.h
  src/util/string_to_number_converter.h
)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  add_library(${LIBRARY_NAME} SHARED ${SRC} ${INC})
else()
  add_library(${LIBRARY_NAME} STATIC ${SRC} ${INC})
endif()

#-----------------------------------------------------

include_directories(${CMAKE_SOURCE_DIR})
include_directories("${CMAKE_SOURCE_DIR}/src")
include_directories("${CMAKE_SOURCE_DIR}/src/authentication")
include_directories("${CMAKE_SOURCE_DIR}/src/authentication/adfs")
include_directories("${CMAKE_SOURCE_DIR}/src/failover")
include_directories("${CMAKE_SOURCE_DIR}/src/host_availability")
include_directories("${CMAKE_SOURCE_DIR}/src/host_selector")
include_directories("${CMAKE_SOURCE_DIR}/src/limitless")
include_directories("${CMAKE_SOURCE_DIR}/src/util")

if(WIN32)
  add_compile_definitions(WIN32)
else()
  add_definitions(-DSQL_WCHART_CONVERT)
endif()

#-----------------------------------------------------

if(ENABLE_UNIT_TESTS)
  add_subdirectory(test/unit_test)
endif()

#-----------------------------------------------------
# Set compiler flags based on the build type

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  message(STATUS "Adding debug compiler flags")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -fno-inline -fno-omit-frame-pointer")
endif()

#-----------------------------------------------------
# macOS specific configuration

if(APPLE OR CMAKE_SYSTEM_NAME STREQUAL "Linux")
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
message(STATUS "AWSSDK_INCLUDE_DIRS: ${AWSSDK_INCLUDE_DIRS}")
message(STATUS "AWSSDK_LINK_LIBRARIES: ${AWSSDK_LINK_LIBRARIES}")
include_directories(${AWSSDK_INCLUDE_DIRS})
target_link_libraries(${LIBRARY_NAME} ${AWSSDK_LINK_LIBRARIES})

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
  if(APPLE)
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
set(GLOG_BUILD_TESTS OFF) # Don't build tests
set(GLOG_BUILD_BENCHMARKS OFF) # Don't build benchmarks
set(GLOG_USE_CMAKE_INSTALL OFF) # Don't install glog
set(BUILD_SHARED_LIBS OFF) # Force static linking.  Crucial for static glog
FetchContent_MakeAvailable(glog)
include_directories(${glog_SOURCE_DIR})
target_link_libraries(${LIBRARY_NAME} glog::glog)

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
target_link_libraries(${LIBRARY_NAME} ${ODBC_LIBRARIES})

#-----------------------------------------------------
# Combine static libraries into the target static library

if(APPLE)
  add_custom_command(
    TARGET ${LIBRARY_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "Combining static libraries into $<TARGET_FILE:${LIBRARY_NAME}>"
    COMMAND libtool -static -o $<TARGET_FILE:${LIBRARY_NAME}> $<TARGET_FILE:${LIBRARY_NAME}> $<TARGET_FILE:glog::glog> ${AWS_SDK_LIBRARIES_EXPANDED}
  )
endif()

if(WIN32)
  add_custom_command(
    TARGET ${LIBRARY_NAME} POST_BUILD
    COMMAND lib /OUT:$<TARGET_FILE:${LIBRARY_NAME}> $<TARGET_FILE:${LIBRARY_NAME}> $<TARGET_FILE:glog::glog> ${AWS_SDK_LIBRARIES_EXPANDED}
    COMMENT "Packaging additional .lib files into $<TARGET_FILE:${LIBRARY_NAME}>"
)
endif()

set_target_properties(${LIBRARY_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)
