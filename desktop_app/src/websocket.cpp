#include "fast_led_teleop/websocket.hpp"
#include <cstdint>

namespace fast_led_teleop::websocket {
EchoServer::EchoServer() {
    // Set message handler
    m_server.set_message_handler(
        [&](websocketpp::connection_hdl hdl, server::message_ptr msg) {
            m_server.send(hdl, msg->get_payload(), msg->get_opcode());
        });

    m_server.init_asio();
};

void EchoServer::run(uint16_t port) {
    m_server.listen(port);
    m_server.start_accept();
    m_server.run();
};

} // namespace fast_led_teleop::websocket