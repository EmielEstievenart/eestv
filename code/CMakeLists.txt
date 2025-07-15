
add_library(eestv_everything 
    eestv/DiscoverableTcpSocket.cpp
    eestv/DiscoveringTcpSocket.cpp
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