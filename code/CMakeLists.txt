
add_library(eestv_everything 
    eestv/net/discoverable_tcp_socket.cpp
    eestv/net/discovering_tcp_socket.cpp
    eestv/net/discoverable.cpp
    eestv/net/discoverable.hpp
    eestv/net/udp_discovery_server.cpp
    eestv/net/udp_discovery_server.hpp
    eestv/net/udp_discovery_client.cpp
    eestv/net/udp_discovery_client.hpp
    eestv/threading/spinlock_mutex.hpp
)

target_include_directories(eestv_everything
    PUBLIC
    .
    ${CMAKE_CURRENT_SOURCE_DIR}/eestv/net
    ${CMAKE_CURRENT_SOURCE_DIR}/eestv/threading
)

target_link_libraries(eestv_everything
    PUBLIC
    Boost::asio
    Boost::system
)