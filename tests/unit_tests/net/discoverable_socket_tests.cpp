// DiscoverableTcpSocketTest.cpp
#include <gtest/gtest.h>
#include "eestv/net/discoverable_tcp_socket.hpp"
#include "eestv/net/discovering_tcp_socket.hpp"
#include "boost/asio.hpp"
#include <thread>
#include <chrono>
#include <future>

namespace
{
const std::string identifier  = "test_identifier";
const unsigned short udp_port = 12345; // Use a high port to avoid conflicts
const unsigned short tcp_port = 0;     // Dynamic
}

class DiscoverableTcpSocketTest : public ::testing::Test
{
protected:
    boost::asio::io_context io_context_;
};

TEST_F(DiscoverableTcpSocketTest, DiscoveryAndConnection)
{
    DiscoverableTcpSocket discoverable(io_context_, identifier, udp_port, tcp_port);
    discoverable.start();

    // Give time for listener to start
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    DiscoveringTcpSocket discovering(io_context_, identifier, udp_port);

    // Use async_connect_via_discovery with a promise/future for synchronous testing
    std::promise<boost::system::error_code> connect_promise;
    std::future<boost::system::error_code> connect_future = connect_promise.get_future();

    discovering.async_connect_via_discovery([&connect_promise](boost::system::error_code ec) { connect_promise.set_value(ec); });

    // Run the IO context briefly to handle the connection
    std::thread connect_io_thread([this]() { io_context_.run_for(std::chrono::seconds(2)); });

    // Wait for the connection to complete
    auto connect_status = connect_future.wait_for(std::chrono::seconds(3));
    ASSERT_EQ(connect_status, std::future_status::ready);

    boost::system::error_code connect_ec = connect_future.get();
    ASSERT_FALSE(connect_ec) << "Connection failed: " << connect_ec.message();

    connect_io_thread.join();

    ASSERT_TRUE(discovering.is_open());

    tcp::socket accepted_socket(io_context_);

    // Use async_accept with a promise/future for synchronous testing
    std::promise<boost::system::error_code> accept_promise;
    std::future<boost::system::error_code> accept_future = accept_promise.get_future();

    discoverable.async_accept(accepted_socket, [&accept_promise](boost::system::error_code ec) { accept_promise.set_value(ec); });

    // Run the IO context briefly to handle the accept
    std::thread io_thread([this]() { io_context_.run_for(std::chrono::milliseconds(500)); });

    // Wait for the accept to complete
    auto status = accept_future.wait_for(std::chrono::seconds(1));
    ASSERT_EQ(status, std::future_status::ready);

    boost::system::error_code accept_ec = accept_future.get();
    ASSERT_FALSE(accept_ec) << "Accept failed: " << accept_ec.message();

    io_thread.join();

    ASSERT_TRUE(accepted_socket.is_open());

    // Cleanup
    discovering.close();
    accepted_socket.close();
}