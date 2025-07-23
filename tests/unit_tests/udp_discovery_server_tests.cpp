#include "eestv/net/discoverable.hpp"
#include "eestv/net/udp_discovery_server.hpp"
#include <boost/asio.hpp>
#include <chrono>
#include <gtest/gtest.h>
#include <string>
#include <thread>


namespace {
const int TEST_PORT = 54321; // Use a high port to avoid conflicts
const std::string TEST_IDENTIFIER = "test_service";
const std::string TEST_REPLY = "Hello from test service!";
const std::string UNKNOWN_IDENTIFIER = "unknown_service";
} // namespace

class UdpDiscoveryTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Create a discoverable service for testing
    test_discoverable = std::make_unique<Discoverable>(
        TEST_IDENTIFIER, []() -> std::string { return TEST_REPLY; });

    // Create the server
    server = std::make_unique<UdpDiscoveryServer>(io_context, TEST_PORT);
    server->add_discoverable(*test_discoverable);
    server->start();

    // Start the IO context in a separate thread
    io_thread = std::thread([this]() {
      while (true) {
        io_context.run_for(std::chrono::seconds(1));
        if (io_context.stopped()) {
          throw std::runtime_error(
              "IO context stopped unexpectedly during test execution.");
        }
      }
    });
  }

  void TearDown() override {
    io_context.stop();
    if (io_thread.joinable()) {
      io_thread.join();
    }
  }

  std::string sendUdpRequest(const std::string &request) {
    boost::asio::io_context client_io;
    boost::asio::ip::udp::socket client_socket(client_io);

    client_socket.open(boost::asio::ip::udp::v4());

    // Send request
    boost::asio::ip::udp::endpoint server_endpoint(
        boost::asio::ip::make_address("127.0.0.1"), TEST_PORT);

    client_socket.send_to(boost::asio::buffer(request), server_endpoint);

    // Receive response with timeout
    std::array<char, 1024> recv_buffer;
    boost::asio::ip::udp::endpoint sender_endpoint;

    // Set a receive timeout
    client_socket.non_blocking(true);

    auto start_time = std::chrono::steady_clock::now();
    const auto timeout = std::chrono::milliseconds(1000);

    while (std::chrono::steady_clock::now() - start_time < timeout) {
      boost::system::error_code ec;
      size_t bytes_received = client_socket.receive_from(
          boost::asio::buffer(recv_buffer), sender_endpoint, 0, ec);

      if (!ec && bytes_received > 0) {
        return std::string(recv_buffer.data(), bytes_received);
      }

      if (ec != boost::asio::error::would_block) {
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
};

TEST_F(UdpDiscoveryTest, DiscoveryRequest) {
  // Send a discovery request
  std::string response = sendUdpRequest(TEST_IDENTIFIER);

  // Check if the response matches the expected reply
  ASSERT_EQ(response, TEST_REPLY);
}
