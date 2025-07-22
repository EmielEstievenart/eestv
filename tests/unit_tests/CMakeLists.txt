cmake_minimum_required(VERSION 3.20)

project(SocketDiscoveryTest)

option(EESTV_ENABLE_TESTS "Enable building unit tests" ON)

if(EESTV_ENABLE_TESTS)
  # Fetch GoogleTest
  include(FetchContent)
  FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
  )
  FetchContent_MakeAvailable(googletest)

  # Add executable for unit tests
  add_executable(unit_tests 
    discoverable_socket_tests.cpp
    udp_discovery_tests.cpp
  )

  # Link libraries (assuming eestv_everything provides sources and Boost)
  target_link_libraries(unit_tests PRIVATE
    eestv_everything
    GTest::gtest_main
  )

  # Enable testing
  enable_testing()
  add_test(NAME DiscoveryTest COMMAND unit_tests)
endif()