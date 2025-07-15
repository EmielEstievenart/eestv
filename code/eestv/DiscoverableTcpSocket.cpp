// DiscoverableTcpSocket.cpp
#include "DiscoverableTcpSocket.hpp"
#include <boost/system/system_error.hpp>

DiscoverableTcpSocket::DiscoverableTcpSocket(
    boost::asio::io_context &io_context, const std::string &identifier,
    unsigned short udp_port, unsigned short tcp_port)
    : _io_context(io_context), _identifier(identifier), _udp_port(udp_port),
      _tcp_port(tcp_port),
      _acceptor(io_context, tcp::endpoint(tcp::v4(), _tcp_port)),
      _udp_socket(io_context, udp::endpoint(udp::v4(), _udp_port)),
      _running(false) {
  if (_tcp_port == 0) {
    _tcp_port = _acceptor.local_endpoint().port();
  }
}

DiscoverableTcpSocket::~DiscoverableTcpSocket() {
  if (_running) {
    _running = false;
    _udp_socket.close();
    if (_udp_thread.joinable()) {
      _udp_thread.join();
    }
  }
  _acceptor.close();
}

void DiscoverableTcpSocket::start() {
  if (_running)
    return;
  _running = true;
  _udp_thread = std::thread([this]() { udp_listener(); });
}

void DiscoverableTcpSocket::udp_listener() {
  char data[1024] = {0};
  while (_running) {
    udp::endpoint sender;
    boost::system::error_code ec;
    size_t len = _udp_socket.receive_from(boost::asio::buffer(data, 1024),
                                          sender, 0, ec);
    if (ec || !_running)
      continue;
    std::string recv_str(data, len);
    if (recv_str == _identifier) {
      std::string response = std::to_string(_tcp_port);
      _udp_socket.send_to(boost::asio::buffer(response), sender, 0, ec);
    }
  }
}

void DiscoverableTcpSocket::accept(tcp::socket &socket) {
  _acceptor.accept(socket);
}