#pragma once

#define ASIO_STANDALONE
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

namespace fast_led_teleop::websocket {

typedef websocketpp::server<websocketpp::config::asio> server;

struct EchoServer {
    EchoServer();
    void run(uint16_t port);

  private:
    server m_server;
};
} // namespace fast_led_teleop::websocket
