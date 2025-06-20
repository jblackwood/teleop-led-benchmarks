#include "websocket.hpp"
#include <cstdint>
#include <nlohmann/json.hpp> 
using json = nlohmann::json;

namespace fast_led_teleop::websocket {
EchoServer::EchoServer(asio::io_context *io_service ) {
    // Set message handler
    m_server.set_message_handler(
        [&m_server=this->m_server](websocketpp::connection_hdl hdl, server::message_ptr msg) {
            auto payload = msg->get_payload();
            // auto jsonData = json::parse(payload);
            std::cout << payload << std::endl;
            m_server.send(hdl, payload, msg->get_opcode());
        });

    m_server.init_asio(io_service);
};

void EchoServer::run(uint16_t port) {
    m_server.listen(port);
    m_server.start_accept();
    m_server.run();
};

} // namespace fast_led_teleop::websocket