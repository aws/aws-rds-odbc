project(unit_test)

cmake_minimum_required(VERSION 3.30 FATAL_ERROR)
set(CMAKE_CXX_STANDARD 17)

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
  unit_test

  # Runner / Utility 
  main.cc
  mock_objects.h

  # Test Suites
  cluster_topology_info_test.cc
  host_info_test.cc
)

include_directories("${CMAKE_SOURCE_DIR}/src/failover")

target_link_libraries(
  unit_test
  gtest_main
  gmock_main
  aws-rds-odbc
)

add_test(NAME unit_test COMMAND unit_test)
set_target_properties(unit_test PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
