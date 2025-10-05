# Discoverable Sockets User Guide

## Overview

The discoverable sockets library provides automatic service discovery for TCP connections. Servers advertise themselves via UDP, and clients automatically discover and connect without needing to know IP addresses or ports in advance.

## Components

**DiscoverableTcpSocket** - Server that advertises itself via UDP  
**DiscoveringTcpSocket** - Client that discovers and connects to servers

## Architecture

### Component Structure

```mermaid
graph TB
    subgraph "Server Side"
        DTS[DiscoverableTcpSocket]
        UDS[UdpDiscoveryServer]
        DISC[Discoverable]
        TCP_ACC[TCP Acceptor]
        
        DTS -->|owns| UDS
        DTS -->|owns| DISC
        DTS -->|owns| TCP_ACC
        UDS -->|registers| DISC
    end
    
    subgraph "Client Side"
        DTCS[DiscoveringTcpSocket]
        UDC[UdpDiscoveryClient]
        TCP_SOCK[TCP Socket]
        
        DTCS -->|owns| UDC
        DTCS -->|inherits from| TCP_SOCK
    end
    
    UDC -.->|UDP broadcast| UDS
    UDS -.->|UDP response| UDC
    TCP_SOCK -->|TCP connect| TCP_ACC
    
    style DTS fill:#ffcccc
    style DTCS fill:#ccccff
    style UDS fill:#ffffcc
    style UDC fill:#ffffcc
    style TCP_ACC fill:#ccffcc
    style TCP_SOCK fill:#ccffcc
```

### Discovery Protocol Flow

```mermaid
sequenceDiagram
    participant App as Application
    participant DTS as DiscoverableTcpSocket
    participant UDS as UdpDiscoveryServer
    participant Net as Network
    participant UDC as UdpDiscoveryClient
    participant DTCS as DiscoveringTcpSocket
    
    App->>DTS: Create("service_name", udp_port, tcp_port=0)
    DTS->>UDS: Create UdpDiscoveryServer
    DTS->>DTS: Create TCP Acceptor (dynamic port)
    App->>DTS: start()
    DTS->>UDS: add_discoverable("service_name", tcp_port_callback)
    UDS->>Net: Listen on UDP port
    
    Note over DTCS,UDC: Client Side
    App->>DTCS: Create("service_name", udp_port)
    App->>DTCS: async_connect_via_discovery(handler)
    DTCS->>UDC: Create UdpDiscoveryClient
    UDC->>Net: Broadcast "service_name"
    
    Net->>UDS: Discovery request
    UDS->>DTS: Lookup "service_name"
    DTS->>UDS: Return TCP port
    UDS->>Net: Reply with TCP port
    
    Net->>UDC: Receive TCP port + IP
    UDC->>DTCS: Received port
    DTCS->>DTS: TCP connect to discovered port
    DTS->>DTCS: Accept connection
    DTCS->>App: Call handler(success)
```

### Network Communication

```mermaid
graph LR
    subgraph "Server Machine: 192.168.1.10"
        S[DiscoverableTcpSocket<br/>'my_service']
        UDP_S[UDP Server<br/>Port 12345]
        TCP_S[TCP Server<br/>Port 54321<br/>auto-assigned]
    end
    
    subgraph "Client Machine: 192.168.1.20"
        C[DiscoveringTcpSocket<br/>'my_service']
        UDP_C[UDP Client<br/>Port 12345]
    end
    
    UDP_C -->|"1. Broadcast: 'my_service'?"| UDP_S
    UDP_S -->|"2. Reply: '54321'"| UDP_C
    C -->|"3. TCP Connect to<br/>192.168.1.10:54321"| TCP_S
    
    style S fill:#ffcccc
    style C fill:#ccccff
    style UDP_S fill:#ffffcc
    style UDP_C fill:#ffffcc
    style TCP_S fill:#ccffcc
```

## Usage

### Server

```cpp
boost::asio::io_context io_context;

// Create discoverable server (tcp_port=0 for auto-assign)
DiscoverableTcpSocket server(io_context, "my_service", 12345, 0);
server.start();

// Accept connections
boost::asio::ip::tcp::socket socket(io_context);
server.async_accept(socket, [](boost::system::error_code ec) {
    // Handle connection
});

io_context.run();
```

### Client

```cpp
boost::asio::io_context io_context;

// Create discovering client
DiscoveringTcpSocket client(io_context, "my_service", 12345);

// Discover and connect
client.async_connect_via_discovery([](boost::system::error_code ec) {
    if (!ec) {
        // Connected - use client as regular TCP socket
    }
});

io_context.run();
```

## Key Points

- **Service Identifier**: Both server and client must use the same identifier string (case-sensitive)
- **UDP Port**: Both must use the same UDP discovery port (fixed, known in advance)
- **TCP Port**: Server can use `0` for automatic assignment by OS
- **Retry**: Client automatically retries discovery until successful
- **Thread-Safe**: Uses Boost.Asio's io_context for thread safety
