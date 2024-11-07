project(unit_test)

cmake_minimum_required(VERSION 3.30 FATAL_ERROR)
set(CMAKE_CXX_STANDARD 20)

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

  host_availability/simple_host_availability_strategy_test.cc
  host_selector/random_host_selector_test.cc
)

#-----------------------------------------------------

target_link_libraries(
  # Name
  ${PROJECT_NAME}

  # External Libaries
  gtest_main
  gmock_main

  # Source Libaries
  aws-rds-odbc
)

#-----------------------------------------------------

add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})
set_target_properties(${PROJECT_NAME} PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
