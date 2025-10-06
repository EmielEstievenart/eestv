#include <iostream>
#include <vector>
#include <string>

#include "eestv/logging/eestv_logging.hpp"

void test_log_levels()
{
    std::cout << "\n=== Testing all log levels ===\n";

    EESTV_LOG_ERROR("This is an ERROR message");
    EESTV_LOG_INFO("This is an INFO message");
    EESTV_LOG_DEBUG("This is a DEBUG message");
    EESTV_LOG_TRACE("This is a TRACE message");

    std::cout << "\n=== Testing with stream expressions ===\n";

    int number       = 42;
    std::string name = "eestv";
    double pi        = 3.14159;

    EESTV_LOG_ERROR("Error: Failed to process " << number << " items for " << name);
    EESTV_LOG_INFO("Info: Processing " << number << " items with value " << pi);
    EESTV_LOG_DEBUG("Debug: Variable values - number=" << number << ", name=" << name << ", pi=" << pi);
    EESTV_LOG_TRACE("Trace: Entering function with parameters (" << number << ", \"" << name << "\", " << pi << ")");
}

int main(int argc, char* argv[])
{
    std::vector<std::string> args(argv, argv + argc);

    if (argc > 1)
    {
        if (args[1] == "--help" || args[1] == "-h")
        {
            std::cout << "Usage: " << args[0] << " [options]\n"
                      << "  --help, -h    Show this help message\n";
            return 0;
        }
    }

    std::cout << "eestv data_bridge example\n";
    std::cout << "Arguments (" << argc << "):\n";
    for (int i = 0; i < argc; ++i)
    {
        std::cout << "  [" << i << "] " << args[i] << "\n";
    }

    // Test logging with different log levels
    std::cout << "\n=== Testing with ERROR level ===\n";
    EESTV_SET_LOG_LEVEL(Error);
    test_log_levels();

    std::cout << "\n=== Testing with INFO level ===\n";
    EESTV_SET_LOG_LEVEL(Info);
    test_log_levels();

    std::cout << "\n=== Testing with DEBUG level ===\n";
    EESTV_SET_LOG_LEVEL(Debug);
    test_log_levels();

    std::cout << "\n=== Testing with TRACE level ===\n";
    EESTV_SET_LOG_LEVEL(Trace);
    test_log_levels();

    std::cout << "\n=== Logging test completed ===\n";

    return 0;
}