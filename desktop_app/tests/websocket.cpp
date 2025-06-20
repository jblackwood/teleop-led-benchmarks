#include "websocket.hpp"
#include <asio/io_context.hpp>
#include <gtest/gtest.h>
#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>

#define ASIO_STANDALONE
namespace websocket = fast_led_teleop::websocket;
typedef websocketpp::client<websocketpp::config::asio_client> client;

void runServer(asio::io_context &io){
    websocket::EchoServer server{&io};
    std::cout << "WebSocket echo server listening on port 9002..." << std::endl;
    server.run(9002); 
}

TEST(WebsocketTests, Simple) {
    auto server_context = asio::io_context{};
    std::thread server_thread(runServer, std::ref(server_context));
    auto client_context = asio::io_context();
    client c;

    // Use custom io_context
    c.init_asio(&client_context);

    c.set_open_handler([&](websocketpp::connection_hdl hdl) {
        std::cout << "Connected\n";
        c.send(hdl, "Hello using custom io_context!", websocketpp::frame::opcode::text);
    });

    c.set_message_handler([&](websocketpp::connection_hdl, client::message_ptr msg) {
        std::cout << "Received: " << msg->get_payload() << "\n";
        c.stop();
    });

    websocketpp::lib::error_code ec;
    auto con = c.get_connection("ws://localhost:9002", ec);
    ASSERT_FALSE(ec);

    c.connect(con);

    // Run the shared context
    client_context.run();
    server_context.stop();
    server_thread.join();
}
