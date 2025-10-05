#include "eestv/net/discovery/discoverable.hpp"
#include "eestv/net/discovery/udp_discovery_client.hpp"
#include "eestv/net/discovery/udp_discovery_server.hpp"
#include <boost/asio.hpp>
#include <chrono>
#include <gtest/gtest.h>
#include <string>
#include <thread>
#include <atomic>

namespace
{
const int test_port                    = 54322; // Different port from existing tests
const std::string test_service1        = "database_service";
const std::string test_service2        = "api_service";
const std::string test_reply1          = "127.0.0.1:5432";
const std::string test_reply2          = "127.0.0.1:8080";
const std::string non_existent_service = "missing_service";
} // namespace

class DiscoveryIntegrationTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Server will be created in individual tests as needed
        io_thread = std::thread(
            [this]()
            {
                auto work_guard = boost::asio::make_work_guard(io_context);
                io_context.run();
            });

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    void TearDown() override
    {
        if (client)
        {
            client->stop();
            client.reset();
        }

        if (server)
        {
            server.reset();
        }

        io_context.stop();

        if (io_thread.joinable())
        {
            io_thread.join();
        }

        io_context.restart();
    }

protected:
    boost::asio::io_context io_context;
    std::unique_ptr<UdpDiscoveryServer> server;
    std::unique_ptr<UdpDiscoveryClient> client;
    std::thread io_thread;
};

/**
 * Tests basic discovery flow: server with one service, client finds it
 */
TEST_F(DiscoveryIntegrationTest, SingleServiceDiscovery)
{
    // Setup server with one discoverable service
    Discoverable service(test_service1, []() { return test_reply1; });
    server = std::make_unique<UdpDiscoveryServer>(io_context, test_port);
    server->add_discoverable(service);
    server->start();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Setup client to discover the service
    std::atomic<bool> found {false};
    std::string received_reply;

    client =
        std::make_unique<UdpDiscoveryClient>(io_context, test_service1, std::chrono::milliseconds(500), test_port,
                                             [&found, &received_reply](const std::string& response, const boost::asio::ip::udp::endpoint&)
                                             {
                                                 received_reply = response;
                                                 found          = true;
                                                 return true;
                                             });

    client->start();

    // Wait for discovery to complete
    auto start_time = std::chrono::steady_clock::now();
    while (!found && std::chrono::steady_clock::now() - start_time < std::chrono::seconds(2))
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    ASSERT_TRUE(found) << "Service was not discovered within timeout";
    EXPECT_EQ(received_reply, test_reply1);
}

/**
 * Tests server with multiple services, client discovers the correct one
 */
TEST_F(DiscoveryIntegrationTest, MultipleServicesDiscovery)
{
    // Setup server with multiple discoverable services
    Discoverable service1(test_service1, []() { return test_reply1; });
    Discoverable service2(test_service2, []() { return test_reply2; });

    server = std::make_unique<UdpDiscoveryServer>(io_context, test_port);
    server->add_discoverable(service1);
    server->add_discoverable(service2);
    server->start();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Test discovery of first service
    {
        std::atomic<bool> found {false};
        std::string received_reply;

        client = std::make_unique<UdpDiscoveryClient>(
            io_context, test_service1, std::chrono::milliseconds(500), test_port,
            [&found, &received_reply](const std::string& response, const boost::asio::ip::udp::endpoint&)
            {
                received_reply = response;
                found          = true;
                return true;
            });

        client->start();

        auto start_time = std::chrono::steady_clock::now();
        while (!found && std::chrono::steady_clock::now() - start_time < std::chrono::seconds(2))
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        ASSERT_TRUE(found) << "First service was not discovered";
        EXPECT_EQ(received_reply, test_reply1);

        client->stop();
        client.reset();
    }

    // Test discovery of second service
    {
        std::atomic<bool> found {false};
        std::string received_reply;

        client = std::make_unique<UdpDiscoveryClient>(
            io_context, test_service2, std::chrono::milliseconds(500), test_port,
            [&found, &received_reply](const std::string& response, const boost::asio::ip::udp::endpoint&)
            {
                received_reply = response;
                found          = true;
                return true;
            });

        client->start();

        auto start_time = std::chrono::steady_clock::now();
        while (!found && std::chrono::steady_clock::now() - start_time < std::chrono::seconds(2))
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        ASSERT_TRUE(found) << "Second service was not discovered";
        EXPECT_EQ(received_reply, test_reply2);
    }
}

/**
 * Tests client behavior when service does not exist
 */
TEST_F(DiscoveryIntegrationTest, NonexistentServiceNoResponse)
{
    // Setup server with one service
    Discoverable service(test_service1, []() { return test_reply1; });
    server = std::make_unique<UdpDiscoveryServer>(io_context, test_port);
    server->add_discoverable(service);
    server->start();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Try to discover a nonexistent service
    std::atomic<bool> found {false};
    std::atomic<int> retry_count {0};

    client = std::make_unique<UdpDiscoveryClient>(io_context, non_existent_service, std::chrono::milliseconds(300), test_port,
                                                  [&found, &retry_count](const std::string&, const boost::asio::ip::udp::endpoint&)
                                                  {
                                                      found = true;
                                                      retry_count++;
                                                      return true;
                                                  });

    client->start();

    // Wait for a reasonable time to verify no response
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    EXPECT_FALSE(found) << "Unexpectedly received response for nonexistent service";
    EXPECT_EQ(retry_count, 0) << "Handler should not be called for nonexistent service";
}

/**
 * Tests dynamic callback - reply changes based on state
 */
TEST_F(DiscoveryIntegrationTest, DynamicCallbackReply)
{
    // Use a counter to generate dynamic replies
    int call_count = 0;
    Discoverable service(test_service1, [&call_count]() { return "reply_" + std::to_string(++call_count); });

    server = std::make_unique<UdpDiscoveryServer>(io_context, test_port);
    server->add_discoverable(service);
    server->start();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // First discovery
    {
        std::atomic<bool> found {false};
        std::string received_reply;

        client = std::make_unique<UdpDiscoveryClient>(
            io_context, test_service1, std::chrono::milliseconds(500), test_port,
            [&found, &received_reply](const std::string& response, const boost::asio::ip::udp::endpoint&)
            {
                received_reply = response;
                found          = true;
                return true;
            });

        client->start();

        auto start_time = std::chrono::steady_clock::now();
        while (!found && std::chrono::steady_clock::now() - start_time < std::chrono::seconds(2))
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        ASSERT_TRUE(found);
        EXPECT_EQ(received_reply, "reply_1");

        client->stop();
        client.reset();
    }

    // Second discovery should get a different reply
    {
        std::atomic<bool> found {false};
        std::string received_reply;

        client = std::make_unique<UdpDiscoveryClient>(
            io_context, test_service1, std::chrono::milliseconds(500), test_port,
            [&found, &received_reply](const std::string& response, const boost::asio::ip::udp::endpoint&)
            {
                received_reply = response;
                found          = true;
                return true;
            });

        client->start();

        auto start_time = std::chrono::steady_clock::now();
        while (!found && std::chrono::steady_clock::now() - start_time < std::chrono::seconds(2))
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        ASSERT_TRUE(found);
        EXPECT_EQ(received_reply, "reply_2");
    }
}

/**
 * Tests client retry mechanism
 */
TEST_F(DiscoveryIntegrationTest, ClientRetryMechanism)
{
    // Start server with a delay
    std::thread delayed_server_thread(
        [this]()
        {
            // Wait before starting server
            std::this_thread::sleep_for(std::chrono::milliseconds(800));

            Discoverable service(test_service1, []() { return test_reply1; });
            server = std::make_unique<UdpDiscoveryServer>(io_context, test_port);
            server->add_discoverable(service);
            server->start();
        });

    // Start client immediately (server not ready yet)
    std::atomic<bool> found {false};
    std::string received_reply;

    client =
        std::make_unique<UdpDiscoveryClient>(io_context, test_service1, std::chrono::milliseconds(300), test_port,
                                             [&found, &received_reply](const std::string& response, const boost::asio::ip::udp::endpoint&)
                                             {
                                                 received_reply = response;
                                                 found          = true;
                                                 return true;
                                             });

    client->start();

    // Wait for discovery to complete (should retry and eventually find the service)
    auto start_time = std::chrono::steady_clock::now();
    while (!found && std::chrono::steady_clock::now() - start_time < std::chrono::seconds(3))
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    delayed_server_thread.join();

    ASSERT_TRUE(found) << "Service was not discovered despite retries";
    EXPECT_EQ(received_reply, test_reply1);
}

/**
 * Tests server handling multiple concurrent requests
 */
TEST_F(DiscoveryIntegrationTest, ConcurrentClientRequests)
{
    // Setup server
    Discoverable service(test_service1, []() { return test_reply1; });
    server = std::make_unique<UdpDiscoveryServer>(io_context, test_port);
    server->add_discoverable(service);
    server->start();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Create multiple clients simultaneously
    const int num_clients = 5;
    std::vector<std::atomic<bool>> results(num_clients);
    std::vector<std::string> replies(num_clients);

    std::vector<boost::asio::io_context> client_contexts(num_clients);
    std::vector<std::unique_ptr<UdpDiscoveryClient>> clients(num_clients);
    std::vector<std::thread> client_threads;

    for (int i = 0; i < num_clients; ++i)
    {
        results[i] = false;

        client_threads.emplace_back(
            [&, i]()
            {
                auto work_guard = boost::asio::make_work_guard(client_contexts[i]);

                clients[i] = std::make_unique<UdpDiscoveryClient>(
                    client_contexts[i], test_service1, std::chrono::milliseconds(500), test_port,
                    [&results, &replies, i](const std::string& response, const boost::asio::ip::udp::endpoint&)
                    {
                        replies[i] = response;
                        results[i] = true;
                        return true;
                    });

                clients[i]->start();
                client_contexts[i].run();
            });
    }

    // Wait for all clients to complete
    auto start_time = std::chrono::steady_clock::now();
    bool all_found  = false;
    while (!all_found && std::chrono::steady_clock::now() - start_time < std::chrono::seconds(3))
    {
        all_found = true;
        for (int i = 0; i < num_clients; ++i)
        {
            if (!results[i])
            {
                all_found = false;
                break;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    // Stop all clients
    for (int i = 0; i < num_clients; ++i)
    {
        if (clients[i])
        {
            clients[i]->stop();
        }
        client_contexts[i].stop();
    }

    for (auto& thread : client_threads)
    {
        if (thread.joinable())
        {
            thread.join();
        }
    }

    // Verify all clients received responses
    for (int i = 0; i < num_clients; ++i)
    {
        EXPECT_TRUE(results[i]) << "Client " << i << " did not receive response";
        EXPECT_EQ(replies[i], test_reply1) << "Client " << i << " received incorrect reply";
    }
}
