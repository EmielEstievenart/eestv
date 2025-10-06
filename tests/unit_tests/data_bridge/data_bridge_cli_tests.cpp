#include "eestv/data_bridge/data_bridge.hpp"
#include "eestv/logging/eestv_logging.hpp"

#include <boost/program_options/errors.hpp>
#include <gtest/gtest.h>

#include <initializer_list>
#include <string>
#include <vector>

namespace
{
class LogLevelGuard
{
public:
    LogLevelGuard() : _previous(eestv::logging::current_log_level) { }
    LogLevelGuard(const LogLevelGuard&)            = delete;
    LogLevelGuard& operator=(const LogLevelGuard&) = delete;
    LogLevelGuard(LogLevelGuard&&)                 = delete;
    LogLevelGuard& operator=(LogLevelGuard&&)      = delete;

    ~LogLevelGuard() { eestv::logging::current_log_level = _previous; }

private:
    eestv::logging::LogLevel _previous;
};

class ArgumentBuffer
{
public:
    ArgumentBuffer(std::initializer_list<const char*> arguments)
    {
        _storage.reserve(arguments.size());
        for (const char* item : arguments)
        {
            _storage.emplace_back(item);
        }

        _argv.reserve(_storage.size());
        for (std::string& argument : _storage)
        {
            _argv.emplace_back(argument.data());
        }
    }

    [[nodiscard]] int argc() const noexcept { return static_cast<int>(_argv.size()); }

    [[nodiscard]] char** argv() noexcept { return _argv.data(); }

private:
    std::vector<std::string> _storage;
    std::vector<char*> _argv;
};
} // namespace

TEST(DataBridgeCliTests, ParsesClientEndpointWithInfoVerbosity)
{
    LogLevelGuard guard;
    eestv::logging::current_log_level = eestv::logging::LogLevel::Trace;

    ArgumentBuffer arguments {"data_bridge", "--client", "--endpoint", "--discovery", "alpha"};

    eestv::DataBridge data_bridge(arguments.argc(), arguments.argv());

    EXPECT_EQ(eestv::ClientServerMode::client, data_bridge.client_server_mode());
    EXPECT_EQ(eestv::EndpointMode::endpoint, data_bridge.endpoint_mode());
    EXPECT_EQ("alpha", data_bridge.discovery_target());
    EXPECT_EQ(eestv::logging::LogLevel::Info, eestv::logging::current_log_level);
}

TEST(DataBridgeCliTests, ParsesDebugVerbosity)
{
    LogLevelGuard guard;
    eestv::logging::current_log_level = eestv::logging::LogLevel::Error;

    ArgumentBuffer arguments {"data_bridge", "-v", "--server", "--endpoint", "--discovery", "beta"};

    eestv::DataBridge data_bridge(arguments.argc(), arguments.argv());

    EXPECT_EQ(eestv::ClientServerMode::server, data_bridge.client_server_mode());
    EXPECT_EQ(eestv::EndpointMode::endpoint, data_bridge.endpoint_mode());
    EXPECT_EQ("beta", data_bridge.discovery_target());
    EXPECT_EQ(eestv::logging::LogLevel::Debug, eestv::logging::current_log_level);
}

TEST(DataBridgeCliTests, ParsesTraceVerbosityAndBridgeMode)
{
    LogLevelGuard guard;
    eestv::logging::current_log_level = eestv::logging::LogLevel::Error;

    ArgumentBuffer arguments {"data_bridge", "-vv", "--client", "--bridge", "--discovery", "gamma"};

    eestv::DataBridge data_bridge(arguments.argc(), arguments.argv());

    EXPECT_EQ(eestv::ClientServerMode::client, data_bridge.client_server_mode());
    EXPECT_EQ(eestv::EndpointMode::bridge, data_bridge.endpoint_mode());
    EXPECT_EQ("gamma", data_bridge.discovery_target());
    EXPECT_EQ(eestv::logging::LogLevel::Trace, eestv::logging::current_log_level);
}

TEST(DataBridgeCliTests, ThrowsWhenBothClientAndServer)
{
    LogLevelGuard guard;

    ArgumentBuffer arguments {"data_bridge", "--client", "--server", "--endpoint", "--discovery", "delta"};

    EXPECT_THROW((void)eestv::DataBridge(arguments.argc(), arguments.argv()), boost::program_options::error);
}

TEST(DataBridgeCliTests, ThrowsWhenBothEndpointAndBridge)
{
    LogLevelGuard guard;

    ArgumentBuffer arguments {"data_bridge", "--client", "--endpoint", "--bridge", "--discovery", "epsilon"};

    EXPECT_THROW((void)eestv::DataBridge(arguments.argc(), arguments.argv()), boost::program_options::error);
}

TEST(DataBridgeCliTests, ThrowsWhenDiscoveryMissing)
{
    LogLevelGuard guard;

    ArgumentBuffer arguments {"data_bridge", "--client", "--endpoint"};

    EXPECT_THROW((void)eestv::DataBridge(arguments.argc(), arguments.argv()), boost::program_options::error);
}