cmake_minimum_required(VERSION 3.20)

# Automatically collect all source files in this directory
file(GLOB SOURCES CONFIGURE_DEPENDS *.cpp *.cc *.cxx)

# Create the overloading example executable
add_executable(overloading_example ${SOURCES})

# Set C++ standard
target_compile_features(overloading_example PRIVATE cxx_std_17)

# Add compiler-specific flags for better warnings and debugging
if(MSVC)
  target_compile_options(overloading_example PRIVATE /W4)
else()
  target_compile_options(overloading_example PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Ensure this folder is in the include path
target_include_directories(overloading_example
                           PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Set output directory (optional)
set_target_properties(overloading_example PROPERTIES RUNTIME_OUTPUT_DIRECTORY
                                                     ${CMAKE_BINARY_DIR}/bin)
