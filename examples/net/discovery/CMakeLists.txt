cmake_minimum_required(VERSION 3.10)

# Server executable
add_executable(discovery_server_example server.cpp)
target_link_libraries(discovery_server_example PRIVATE eestv_lib)

# Client executable
add_executable(discovery_client_example client.cpp)
target_link_libraries(discovery_client_example PRIVATE eestv_lib)
