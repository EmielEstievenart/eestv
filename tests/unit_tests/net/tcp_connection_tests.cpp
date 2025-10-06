#include "eestv/net/client_connection.hpp"
#include "eestv/net/server_connection.hpp"
#include "eestv/net/tcp_server.hpp"
#include <boost/asio.hpp>
#include <gtest/gtest.h>

// TODO: Add tests for:
// - ClientConnection reconnection behavior
// - ServerConnection lifecycle
// - TcpServer accept/reject patterns
// - Custom buffer types with Connection templates
// - Connection state transitions
// - Error handling and edge cases
