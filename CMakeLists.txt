cmake_minimum_required(VERSION 3.21)

# Specify which Boost libraries to include (limits build to essentials; add more if needed)
set(BOOST_INCLUDE_LIBRARIES asio system)  # 'system' is often needed for ASIO error handling
set(BOOST_ENABLE_CMAKE ON)

# Add Boost as a subdirectory using the env var
add_subdirectory($ENV{BOOST_ROOT} my_boost)

project(academic)

add_subdirectory(code)
add_subdirectory(examples)
add_subdirectory(tests/unit_tests)