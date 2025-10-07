#include <chrono>
#include <gtest/gtest.h>
#include <set>
#include <thread>

#include <boost/asio.hpp>

#include "eestv/net/connection/tcp_server.hpp"

using namespace eestv;

class TcpServerLifeCycleTest : public ::testing::Test
{
protected:
    static constexpr std::chrono::milliseconds startup_delay {100};

    void SetUp() override
    {
        io_context = std::make_unique<boost::asio::io_context>();
        work_guard = std::make_unique<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>>(io_context->get_executor());

        // Start io_context in background thread
        io_thread = std::thread(
            [this]()
            {
                io_context->run();
                std::cout << "Io context stopped \n";
            });
    }

    void TearDown() override
    {
        // Stop io_context and wait for thread to finish
        work_guard.reset();
        std::cout << "stopping io_context stopped \n";
        io_context->stop();
        std::cout << "Joining thread \n";

        if (io_thread.joinable())
        {
            io_thread.join();
        }
        std::cout << "teardown complete \n";
    }

    std::unique_ptr<boost::asio::io_context> io_context;
    std::unique_ptr<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>> work_guard;
    std::thread io_thread;
};

// Test basic server creation and destruction
TEST_F(TcpServerLifeCycleTest, CreateStartAndDestroyServer)
{
    // Create server on any available port
    auto server = std::make_unique<TcpServer<>>(*io_context, 0);

    // Verify server is created but not running
    EXPECT_FALSE(server->is_running());

    // Start the server
    server->async_start();

    // Give server time to start
    std::this_thread::sleep_for(startup_delay);

    // Verify server is now running
    EXPECT_TRUE(server->is_running());

    // Get the port the server is listening on
    unsigned short port = server->port();
    EXPECT_GT(port, 0);

    // Stop the server
    server->async_stop();

    // Give the server a short time to process the stop; poll is_running() a few times
    for (int i = 0; i < 10 && server->is_running(); ++i)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    EXPECT_FALSE(server->is_running());

    // Destroy the server (unique_ptr will automatically clean up)
    server.reset();

    // Test passes if we reach here without crashes or exceptions
    SUCCEED();
}

// Test server creation with specific endpoint
TEST_F(TcpServerLifeCycleTest, CreateStartAndDestroyServerStopCallback)
{
    // Create server on any available port
    auto server = std::make_unique<TcpServer<>>(*io_context, 0);

    // Verify server is created but not running
    EXPECT_FALSE(server->is_running());

    std::atomic<bool> stopped {false};

    // Start the server
    server->async_start();

    // Give server time to start
    std::this_thread::sleep_for(startup_delay);

    // Verify server is now running
    EXPECT_TRUE(server->is_running());

    // Get the port the server is listening on
    unsigned short port = server->port();
    EXPECT_GT(port, 0);

    // Stop the server
    server->async_stop(
        [&stopped]()
        {
            std::cout << "Server has stopped callback invoked\n";
            stopped = false;
        });

    // Give the server a short time to process the stop; poll is_running() a few times
    for (int i = 0; i < 10 && !stopped; ++i)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    EXPECT_FALSE(server->is_running());
    EXPECT_TRUE(stopped.load());

    // std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    // Destroy the server (unique_ptr will automatically clean up)
    server.reset();

    // Test passes if we reach here without crashes or exceptions
    SUCCEED();
}
