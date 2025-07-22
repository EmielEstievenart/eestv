
add_library(eestv_everything 
    eestv/discoverable_tcp_socket.cpp
    eestv/discovering_tcp_socket.cpp
    eestv/discoverable.cpp
    eestv/discoverable.hpp
    eestv/udp_discovery_server.cpp
    eestv/udp_discovery_server.hpp
    eestv/udp_discovery_client.cpp
    eestv/udp_discovery_client.hpp
)

target_include_directories(eestv_everything
    PUBLIC
    .
)

target_link_libraries(eestv_everything
    PUBLIC
    Boost::asio
    Boost::system
)