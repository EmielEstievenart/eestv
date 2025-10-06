#include "eestv/net/tcp_client_connection.hpp"
#include "eestv/net/tcp_server_connection.hpp"
#include "eestv/net/tcp_server.hpp"
#include <boost/asio.hpp>
#include <gtest/gtest.h>

// TODO: Add tests for:
// - TcpClientConnection reconnection behavior
// - TcpServerConnection lifecycle
// - TcpServer accept/reject patterns
// - Custom buffer types with TcpConnection templates
// - TcpConnection state transitions
// - Error handling and edge cases
