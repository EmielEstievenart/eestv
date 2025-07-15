// DiscoveringTcpSocket.hpp
#pragma once

#include "boost/asio/io_context.hpp"
#include <boost/asio.hpp>
#include <string>

namespace ba = boost::asio;
using boost::asio::ip::tcp;
using boost::asio::ip::udp;

// Class representing a discovering TCP socket that searches for a discoverable socket via UDP.
class DiscoveringTcpSocket : public tcp::socket {
private:
    std::string _identifier;
    unsigned short _udp_port;
    ba::io_context& _io_context;

public:
    // Constructor to initialize the discovering socket.
    // @param io_context The IO context.
    // @param identifier The friendly identifier.
    // @param udp_port The UDP port.
    DiscoveringTcpSocket(boost::asio::io_context& io_context, const std::string& identifier, unsigned short udp_port);
    
    // Connect to the discovered TCP socket via UDP broadcast.
    void connect_via_discovery();
};