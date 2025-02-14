project(unit_test)

cmake_minimum_required(VERSION 3.22 FATAL_ERROR)
set(CMAKE_CXX_STANDARD 20)

#-----------------------------------------------------

# Create an option for selecting the build type
option(UNICODE_BUILD "Enable Unicode build" OFF)

# Set compiler definitions for the library
if (UNICODE_BUILD)
  add_definitions(-DUNICODE -D_UNICODE)
endif()

# Output a message indicating the build type
message(STATUS "Building ${BUILD_DIR_SUFFIX} version of ${PROJECT_NAME}")

#-----------------------------------------------------

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

#-----------------------------------------------------

include_directories("${CMAKE_SOURCE_DIR}/test")
include_directories("${CMAKE_SOURCE_DIR}/test/unit_test")

add_executable(
  # Executable Name
  ${PROJECT_NAME}

  # Runner / Utility
  main.cc
  mock_objects.h

  # Test Suites
  host_info_test.cc

  authentication/authentication_provider_test.cc
  authentication/adfs/adfs_test.cc
  authentication/okta/okta_test.cc
  authentication/secrets_manager_helper_test.cc

  failover/cluster_topology_info_test.cc
  failover/cluster_topology_monitor_test.cc
  failover/cluster_topology_query_helper_test.cc

  host_availability/simple_host_availability_strategy_test.cc

  host_selector/random_host_selector_test.cc
  host_selector/round_robin_host_selector_test.cc
  host_selector/highest_weight_host_selector_test.cc

  limitless/limitless_monitor_service_test.cc

  util/connection_string_helper_test.cc
  util/sliding_cache_map_test.cc
  util/odbc_helper_test.cc
  util/rds_utils_test.cc
)

#-----------------------------------------------------
# Find required headers and libraries

if (APPLE)
  # Required to ensure unixodbc is found and used before iODBC
  set(ODBC_INCLUDE_DIR /opt/homebrew/opt/unixodbc/include)
  find_package(ODBC REQUIRED)
  
  find_library(ICONV_LIB iconv)
  find_library(LTDL_LIB ltdl)

  include_directories(${ODBC_INCLUDE_DIRS})
  target_link_libraries(${PROJECT_NAME} ${ODBC_LIBRARIES} ${ICONV_LIB} ${LTDL_LIB})
endif()

#-----------------------------------------------------

target_link_libraries(
  ${PROJECT_NAME}

  # External Libaries
  gtest_main
  gmock_main
)

if (UNICODE_BUILD)
  target_link_libraries(${PROJECT_NAME} aws-rds-odbc-w)
else()
  target_link_libraries(${PROJECT_NAME} aws-rds-odbc-a)
endif()


#-----------------------------------------------------

add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})
set_target_properties(${PROJECT_NAME} PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
