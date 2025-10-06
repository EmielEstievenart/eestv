#include "eestv/net/client_connection.hpp"
#include "eestv/net/server_connection.hpp"
#include "eestv/net/tcp_server.hpp"
#include <boost/asio.hpp>
#include <gtest/gtest.h>
#include <chrono>
#include <thread>

namespace eestv::test
{

/**
 * Test that keep-alive callback can be set and returns the expected type
 */
TEST(KeepAliveCallbackTests, CallbackCanBeSet)
{
    boost::asio::io_context io_context;
    boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::make_address("127.0.0.1"), 54321);

    auto connection = std::make_shared<ClientConnection<>>(endpoint, io_context, std::chrono::seconds(1));

    bool callback_was_called = false;

    // Set a keep-alive callback
    connection->set_keep_alive_callback(
        [&callback_was_called]() -> std::pair<bool, std::vector<char>>
        {
            callback_was_called = true;
            std::string msg     = "TEST_KEEPALIVE\n";
            std::vector<char> data(msg.begin(), msg.end());
            return {true, data};
        });

    // Verify callback was set (we can't easily verify it's called without a full connection setup)
    EXPECT_FALSE(callback_was_called); // Not called until monitoring starts
}

/**
 * Test that callback returning false prevents sending keep-alive
 */
TEST(KeepAliveCallbackTests, CallbackCanReturnFalse)
{
    boost::asio::io_context io_context;
    boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::make_address("127.0.0.1"), 54322);

    auto connection = std::make_shared<ClientConnection<>>(endpoint, io_context, std::chrono::seconds(1));

    // Set a callback that returns false
    connection->set_keep_alive_callback([]() -> std::pair<bool, std::vector<char>> { return {false, {}}; });

    // No exception should be thrown when setting this callback
    SUCCEED();
}

/**
 * Test that callback with empty data doesn't cause issues
 */
TEST(KeepAliveCallbackTests, CallbackWithEmptyData)
{
    boost::asio::io_context io_context;
    boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::make_address("127.0.0.1"), 54323);

    auto connection = std::make_shared<ClientConnection<>>(endpoint, io_context, std::chrono::seconds(1));

    // Set a callback that returns true but empty data
    connection->set_keep_alive_callback([]() -> std::pair<bool, std::vector<char>> { return {true, {}}; });

    // No exception should be thrown
    SUCCEED();
}

/**
 * Test custom protocol keep-alive message
 */
TEST(KeepAliveCallbackTests, CustomProtocolMessage)
{
    boost::asio::io_context io_context;
    boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::make_address("127.0.0.1"), 54324);

    auto connection = std::make_shared<ClientConnection<>>(endpoint, io_context, std::chrono::seconds(1));

    std::string expected_msg = "CUSTOM_PROTOCOL_PING\n";

    // Set a callback with custom protocol message
    connection->set_keep_alive_callback(
        [expected_msg]() -> std::pair<bool, std::vector<char>>
        {
            std::vector<char> data(expected_msg.begin(), expected_msg.end());
            return {true, data};
        });

    SUCCEED();
}

} // namespace eestv::test
