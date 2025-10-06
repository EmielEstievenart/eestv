# Network Library User Guide

## Overview

The eestv network library provides components for TCP connections with automatic reconnection, keepalive monitoring, and optional UDP-based service discovery.

## Components

### Connection Classes

The library provides a base `Connection` class and two derived implementations:

- **`Connection`** (abstract base): Provides connection monitoring, keepalive/ping functionality, and connection-lost callbacks
- **`ClientConnection`**: Client-side connection with automatic reconnection using exponential backoff
- **`ServerConnection`**: Server-side connection that marks itself as dead when the connection is lost

See `docs/eestv/net/connection_architecture.md` for detailed architecture documentation.

### Discovery Components

Low-level UDP discovery components are available in `eestv/net/discovery/`:

- **`UdpDiscoveryServer`**: Listens for UDP discovery requests and responds with service information
- **`UdpDiscoveryClient`**: Sends UDP discovery requests to find services on the network
- **`Discoverable`**: Interface for services that can be discovered

These components allow you to implement service discovery patterns where servers advertise themselves via UDP and clients can find them without knowing IP addresses or ports in advance.

## Usage

### Using ClientConnection

```cpp
boost::asio::io_context io_context;

// Create client connection to a remote endpoint
auto connection = std::make_shared<eestv::net::ClientConnection>(
    io_context,
    boost::asio::ip::tcp::endpoint(
        boost::asio::ip::address::from_string("192.168.1.100"), 
        8080
    )
);

// Set connection lost callback
connection->set_connection_lost_callback([](const boost::system::error_code& ec) {
    std::cout << "Connection lost: " << ec.message() << std::endl;
});

// Connect (will auto-reconnect on failure)
connection->connect();

io_context.run();
```

### Using ServerConnection

```cpp
// Accept a connection on the server side
acceptor.async_accept([](boost::system::error_code ec, boost::asio::ip::tcp::socket socket) {
    if (!ec) {
        auto connection = std::make_shared<eestv::net::ServerConnection>(
            std::move(socket)
        );
        
        connection->set_connection_lost_callback([](const boost::system::error_code& ec) {
            // Connection died - clean up this connection
        });
        
        connection->start_monitoring();
    }
});
```

### Using Discovery Components

For service discovery, use the low-level components directly:

**Server side:**
1. Create a TCP acceptor on the desired port (0 for auto-assign)
2. Create a `Discoverable` instance that returns the TCP port when queried
3. Register the `Discoverable` with `UdpDiscoveryServer` and start it

**Client side:**
1. Create a `UdpDiscoveryClient` and start discovery for the desired identifier
2. When a response with a TCP port is received, stop discovery
3. Connect a `ClientConnection` to the discovered endpoint

## Key Points

- **Connection Monitoring**: Both client and server connections support keepalive monitoring
- **Custom Keep-Alive**: Use `set_keep_alive_callback()` to define protocol-specific keep-alive messages
- **Auto-Reconnect**: `ClientConnection` automatically reconnects with exponential backoff
- **Callbacks**: Use connection-lost callbacks to handle disconnection events
- **Service Discovery**: Use the low-level discovery components to build custom discovery patterns
- **Thread-Safe**: All components use Boost.Asio's io_context for thread safety

## Custom Keep-Alive Messages

By default, connections do not send keep-alive messages. To enable keep-alive functionality with protocol-specific messages, set a keep-alive callback:

```cpp
connection->set_keep_alive_callback(
    []() -> std::pair<bool, std::vector<char>>
    {
        // Return {true, data} to send keep-alive data
        // Return {false, {}} to skip this keep-alive cycle
        
        std::string keep_alive_msg = "MY_PROTOCOL_PING\n";
        std::vector<char> data(keep_alive_msg.begin(), keep_alive_msg.end());
        return {true, data};
    });
```

The callback is invoked periodically based on the keep-alive interval (default: 5 seconds). The callback should return:
- **First value (bool)**: Whether to send a keep-alive message in this cycle
- **Second value (vector<char>)**: The data to send if the first value is `true`

This design allows protocols to be built on top of the connection layer without hardcoding specific keep-alive formats.
