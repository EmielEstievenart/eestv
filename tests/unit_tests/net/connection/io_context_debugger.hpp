#pragma once

#include <boost/asio.hpp>
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>

namespace eestv::test
{

/**
 * @brief Helper to debug io_context hanging issues
 * 
 * This class helps identify what's preventing an io_context from completing.
 */
class IoContextDebugger
{
public:
    /**
     * @brief Check if io_context has pending work
     * 
     * @param io_context The io_context to check
     * @return true if there's pending work, false otherwise
     */
    static bool has_pending_work(boost::asio::io_context& io_context)
    {
        // Try to poll - if it returns > 0, there was work to do
        std::size_t handlers_run = io_context.poll();
        return handlers_run > 0 || !io_context.stopped();
    }

    /**
     * @brief Wait for io_context to become idle with timeout
     * 
     * @param io_context The io_context to wait for
     * @param timeout Maximum time to wait
     * @return true if io_context became idle, false if timeout
     */
    static bool wait_for_idle(boost::asio::io_context& io_context, std::chrono::milliseconds timeout)
    {
        auto start = std::chrono::steady_clock::now();

        while (std::chrono::steady_clock::now() - start < timeout)
        {
            if (io_context.stopped())
            {
                return true;
            }

            // Poll to process any pending handlers
            io_context.poll_one();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        return io_context.stopped();
    }

    /**
     * @brief Print diagnostic information about io_context state
     * 
     * @param io_context The io_context to diagnose
     * @param label Optional label for the output
     */
    static void print_state(boost::asio::io_context& io_context, const std::string& label = "")
    {
        std::cout << "=== IoContext State";
        if (!label.empty())
        {
            std::cout << " (" << label << ")";
        }
        std::cout << " ===" << std::endl;

        std::cout << "  Stopped: " << (io_context.stopped() ? "YES" : "NO") << std::endl;

        // Try to poll and see if there are handlers
        std::size_t handlers_run = io_context.poll();
        std::cout << "  Handlers executed by poll(): " << handlers_run << std::endl;

        std::cout << "================================" << std::endl;
    }

    /**
     * @brief Monitor io_context in a background thread and report if it hangs
     * 
     * Starts a thread that periodically checks if the io_context is making progress.
     * Prints warnings if it appears to be stuck.
     * 
     * @param io_context The io_context to monitor
     * @param check_interval How often to check (default: 1 second)
     * @return A function to call to stop monitoring
     */
    static std::function<void()> start_hang_monitor(boost::asio::io_context& io_context,
                                                    std::chrono::milliseconds check_interval = std::chrono::milliseconds(1000))
    {
        auto should_stop    = std::make_shared<std::atomic<bool>>(false);
        auto monitor_thread = std::make_shared<std::thread>(
            [&io_context, check_interval, should_stop]()
            {
                int iterations_without_stop = 0;

                while (!should_stop->load())
                {
                    if (io_context.stopped())
                    {
                        std::cout << "[IoContextMonitor] io_context has stopped cleanly" << std::endl;
                        return;
                    }

                    iterations_without_stop++;

                    if (iterations_without_stop > 5)
                    {
                        std::cout << "[IoContextMonitor] WARNING: io_context still running after "
                                  << (iterations_without_stop * check_interval.count()) << "ms" << std::endl;
                        std::cout << "[IoContextMonitor] Possible causes:" << std::endl;
                        std::cout << "  - Active work_guard preventing shutdown" << std::endl;
                        std::cout << "  - Pending async operations not cancelled" << std::endl;
                        std::cout << "  - Timers still active" << std::endl;
                        std::cout << "  - Sockets/acceptors not closed" << std::endl;
                    }

                    std::this_thread::sleep_for(check_interval);
                }
            });

        // Return a lambda that stops the monitor and joins the thread
        return [should_stop, monitor_thread]()
        {
            should_stop->store(true);
            if (monitor_thread->joinable())
            {
                monitor_thread->join();
            }
        };
    }

    /**
     * @brief Force-stop an io_context and provide diagnostics
     * 
     * This is a last-resort debugging function that will stop the io_context
     * and print information about what might have been keeping it alive.
     * 
     * @param io_context The io_context to force-stop
     */
    static void force_stop_with_diagnostics(boost::asio::io_context& io_context)
    {
        std::cout << "[IoContextDebugger] Force-stopping io_context..." << std::endl;

        if (io_context.stopped())
        {
            std::cout << "[IoContextDebugger] io_context already stopped" << std::endl;
            return;
        }

        std::cout << "[IoContextDebugger] Attempting to process remaining handlers..." << std::endl;
        std::size_t processed = 0;
        while (true)
        {
            std::size_t n = io_context.poll();
            if (n == 0)
            {
                break;
            }
            processed += n;
        }
        std::cout << "[IoContextDebugger] Processed " << processed << " pending handlers" << std::endl;

        if (!io_context.stopped())
        {
            std::cout << "[IoContextDebugger] io_context still not stopped - calling stop()" << std::endl;
            io_context.stop();
        }

        std::cout << "[IoContextDebugger] Final state: " << (io_context.stopped() ? "STOPPED" : "STILL RUNNING (!)") << std::endl;
    }
};

} // namespace eestv::test
