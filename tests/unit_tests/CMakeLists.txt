# Enable testing support
enable_testing()

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

# Discover tests automatically
include(GoogleTest)
gtest_discover_tests(unit_tests WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                                                  DISCOVERY_MODE POST_BUILD)
