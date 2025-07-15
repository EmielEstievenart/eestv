// DiscoverableTcpSocketTest.cpp
#include <gtest/gtest.h>
#include "eestv/DiscoverableTcpSocket.hpp"
#include "eestv/DiscoveringTcpSocket.hpp"
#include <boost/asio.hpp>
#include <thread>
#include <chrono>

namespace {
    const std::string identifier = "test_identifier";
    const unsigned short udp_port = 12345; // Use a high port to avoid conflicts
    const unsigned short tcp_port = 0; // Dynamic
}

class DiscoverableTcpSocketTest : public ::testing::Test {
protected:
    boost::asio::io_context io_context_;
};

TEST_F(DiscoverableTcpSocketTest, DiscoveryAndConnection) {
    DiscoverableTcpSocket discoverable(io_context_, identifier, udp_port, tcp_port);
    discoverable.start();

    // Give time for listener to start
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    DiscoveringTcpSocket discovering(io_context_, identifier, udp_port);
    discovering.connect_via_discovery();

    ASSERT_TRUE(discovering.is_open());

    tcp::socket accepted_socket(io_context_);
    discoverable.accept(accepted_socket);

    ASSERT_TRUE(accepted_socket.is_open());

    // Cleanup
    discovering.close();
    accepted_socket.close();
}