// DiscoverableTcpSocket.hpp
#pragma once

#include <atomic>
#include <boost/asio.hpp>
#include <string>
#include <thread>


namespace ba = boost::asio;
using boost::asio::ip::tcp;
using boost::asio::ip::udp;

// Class representing a discoverable TCP socket that listens for discovery
// requests via UDP broadcasts.
class DiscoverableTcpSocket {
private:
  boost::asio::io_context &_io_context; // Reference to the IO context.
  std::string _identifier;
  unsigned short _udp_port;
  unsigned short _tcp_port;
  tcp::acceptor _acceptor;
  udp::socket _udp_socket;
  std::thread _udp_thread;
  std::atomic<bool> _running;

  // Private method to handle UDP discovery requests.
  void udp_listener();

public:
  // Constructor to initialize the discoverable socket.
  // @param io_context The IO context.
  // @param identifier The friendly identifier.
  // @param udp_port The UDP port.
  // @param tcp_port The TCP port (optional, defaults to 0 for random).
  DiscoverableTcpSocket(boost::asio::io_context &io_context,
                        const std::string &identifier, unsigned short udp_port,
                        unsigned short tcp_port = 0);

  // Destructor to clean up resources.
  ~DiscoverableTcpSocket();

  // Start the discovery listener.
  void start();

  // Accept an incoming TCP connection.
  // @param socket The socket to accept into.
  void accept(tcp::socket &socket);
};