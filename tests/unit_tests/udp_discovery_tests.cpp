#include "eestv/net/discoverable.hpp"
#include "eestv/net/udp_discovery_client.hpp"
#include "eestv/net/udp_discovery_server.hpp"
#include <boost/asio.hpp>
#include <chrono>
#include <gtest/gtest.h>
#include <mswsockdef.h>
#include <string>
#include <thread>

namespace
{
const int TEST_PORT                  = 54321; // Use a high port to avoid conflicts
const std::string TEST_IDENTIFIER    = "test_service";
const std::string TEST_REPLY         = "Hello from test service!";
const std::string TEST_REPLY2        = "Hello from test service 2!";
const std::string UNKNOWN_IDENTIFIER = "unknown_service";
} // namespace

class UdpDiscoveryTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Create a discoverable service for testing
        test_discoverable = std::make_unique<Discoverable>(TEST_IDENTIFIER, []() -> std::string { return TEST_REPLY; });

        // Create the server
        server = std::make_unique<UdpDiscoveryServer>(io_context, TEST_PORT);
        server->add_discoverable(*test_discoverable);
        server->start();

        // Start the IO context in a separate thread
        io_thread = std::thread(
            [this]()
            {
                try
                {
                    // Use work guard to keep io_context alive
                    auto work_guard = boost::asio::make_work_guard(io_context);
                    io_context.run();
                }
                catch (const std::exception& e)
                {
                    std::cerr << "IO thread exception: " << e.what() << std::endl;
                }
            });

        // Give the server time to start
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    void TearDown() override
    {
        // Stop the server first
        if (server)
        {
            server.reset(); // This should clean up server resources
        }

        // Stop the client if it exists
        if (client)
        {
            client->stop();
            client.reset();
        }

        // Stop the IO context
        io_context.stop();

        // Wait for the IO thread to finish
        if (io_thread.joinable())
        {
            io_thread.join();
        }

        // Restart the IO context for the next test
        io_context.restart();
    }

    std::string sendUdpRequest(const std::string& request)
    {
        boost::asio::io_context client_io;
        boost::asio::ip::udp::socket client_socket(client_io);

        client_socket.open(boost::asio::ip::udp::v4());

        // Send request
        boost::asio::ip::udp::endpoint server_endpoint(boost::asio::ip::make_address("127.0.0.1"), TEST_PORT);

        client_socket.send_to(boost::asio::buffer(request), server_endpoint);

        // Receive response with timeout
        std::array<char, 1024> recv_buffer;
        boost::asio::ip::udp::endpoint sender_endpoint;

        // Set a receive timeout
        client_socket.non_blocking(true);

        auto start_time    = std::chrono::steady_clock::now();
        const auto timeout = std::chrono::milliseconds(1000);

        while (std::chrono::steady_clock::now() - start_time < timeout)
        {
            boost::system::error_code ec;
            size_t bytes_received = client_socket.receive_from(boost::asio::buffer(recv_buffer), sender_endpoint, 0, ec);

            if (!ec && bytes_received > 0)
            {
                return std::string(recv_buffer.data(), bytes_received);
            }

            if (ec != boost::asio::error::would_block)
            {
                break;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        return ""; // Timeout or error
    }

protected:
    boost::asio::io_context io_context;
    std::unique_ptr<UdpDiscoveryServer> server;
    std::unique_ptr<Discoverable> test_discoverable;
    std::thread io_thread;
    std::unique_ptr<UdpDiscoveryClient> client;
};

TEST_F(UdpDiscoveryTest, DiscoveryRequest)
{
    // Send a discovery request
    std::string response = sendUdpRequest(TEST_IDENTIFIER);

    // Check if the response matches the expected reply
    ASSERT_EQ(response, TEST_REPLY);
}

TEST_F(UdpDiscoveryTest, DiscoveryClientServer)
{
    bool response_received   = false;
    bool response_is_correct = false;
    std::string received_response;

    client =
        std::make_unique<UdpDiscoveryClient>(io_context, TEST_IDENTIFIER, std::chrono::milliseconds(1000), TEST_PORT,
                                             [&response_received, &response_is_correct, &received_response](const std::string& response)
                                             {
                                                 // For testing, we just print the response
                                                 std::cout << "Received response: " << response << std::endl;
                                                 received_response   = response;
                                                 response_received   = true;
                                                 response_is_correct = (response == TEST_REPLY);
                                                 return true; // Indicate that the response was handled successfully
                                             });

    client->start();

    // Wait for response with timeout
    int retries = 20; // 2 seconds total
    while (!response_received && retries > 0)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        --retries;
    }

    ASSERT_TRUE(response_received) << "No response received within timeout";
    ASSERT_TRUE(response_is_correct) << "Expected '" << TEST_REPLY << "' but got '" << received_response << "'";
}
