cmake_minimum_required(VERSION 3.20)

project(SocketDiscoveryTest)

option(EESTV_ENABLE_TESTS "Enable building unit tests" ON)

if(EESTV_ENABLE_TESTS)
  # Fetch GoogleTest
  include(FetchContent)
  FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip)
  FetchContent_MakeAvailable(googletest)

  # Add executable for unit tests
  add_executable(
    unit_tests net/discoverable_socket_tests.cpp net/udp_discovery_tests.cpp
               data/linear_buffer_tests.cpp serial/serialization_tests.cpp)

  # Link libraries (assuming eestv_lib provides sources and Boost)
  target_link_libraries(unit_tests PRIVATE eestv_lib GTest::gtest_main)

  # Enable testing
  enable_testing()

  # Discover all Google Test cases automatically
  include(GoogleTest)
  gtest_discover_tests(unit_tests)
endif()
