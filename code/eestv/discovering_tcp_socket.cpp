// DiscoveringTcpSocket.cpp
#include "eestv/discovering_tcp_socket.hpp"
#include <boost/system/system_error.hpp>

DiscoveringTcpSocket::DiscoveringTcpSocket(boost::asio::io_context& io_context, const std::string& identifier, unsigned short udp_port)
    : tcp::socket(io_context), _identifier(identifier), _udp_port(udp_port), _io_context(io_context) {}

void DiscoveringTcpSocket::connect_via_discovery() {
    udp::socket udp_sock(_io_context, udp::endpoint(udp::v4(), 0)); // ephemeral port
    udp_sock.set_option(udp::socket::broadcast(true));

    udp::endpoint broadcast_ep(boost::asio::ip::make_address("255.255.255.255"), _udp_port);

    boost::system::error_code ec;
    udp_sock.send_to(boost::asio::buffer(_identifier), broadcast_ep, 0, ec);
    if (ec) throw boost::system::system_error(ec);

    char data[1024];
    udp::endpoint sender;
    size_t len = udp_sock.receive_from(boost::asio::buffer(data, 1024), sender, 0, ec);
    if (ec) throw boost::system::system_error(ec);

    std::string resp(data, len);
    unsigned short discovered_port = std::stoul(resp);

    tcp::endpoint tcp_ep(sender.address(), discovered_port);
    connect(tcp_ep, ec);
    if (ec) throw boost::system::system_error(ec);
}