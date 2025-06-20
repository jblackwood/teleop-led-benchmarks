// #include <asio.hpp>
// #include <iostream>
// #include <string>

// using asio::ip::tcp;

// std::string make_http_response(const std::string& body) {
//     return
//         "HTTP/1.1 200 OK\r\n"
//         "Content-Type: text/plain\r\n"
//         "Content-Length: " + std::to_string(body.size()) + "\r\n"
//         "Connection: close\r\n"
//         "\r\n" + body;
// }

// int main() {
//     try {
//         asio::io_context io_context;
//         tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), 8080));

//         std::cout << "Listening on port 8080...\n";

//         while (true) {
//             tcp::socket socket(io_context);
//             acceptor.accept(socket);

//             // Read request (simple and limited to 1024 bytes)
//             char buffer[1024];
//             asio::error_code ec;
//             std::size_t length = socket.read_some(asio::buffer(buffer), ec);

//             if (!ec) {
//                 std::string request(buffer, length);
//                 std::cout << "Received request:\n" << request << "\n";

//                 std::string response = make_http_response("Hello world");

//                 asio::write(socket, asio::buffer(response), ec);
//             }

//             socket.shutdown(tcp::socket::shutdown_both, ec);
//         }
//     } catch (std::exception& e) {
//         std::cerr << "Exception: " << e.what() << "\n";
//     }

//     return 0;
// }


// #include <asio.hpp>
// #include <iostream>
// #include <thread>
// #include <vector>

// using asio::ip::tcp;

// std::string make_http_response(const std::string& body) {
//     return
//         "HTTP/1.1 200 OK\r\n"
//         "Content-Type: text/plain\r\n"
//         "Content-Length: " + std::to_string(body.size()) + "\r\n"
//         "Connection: close\r\n"
//         "\r\n" + body;
// }

// void handle_client(tcp::socket socket) {
//     try {
//         char buffer[1024];
//         asio::error_code ec;

//         std::size_t length = socket.read_some(asio::buffer(buffer), ec);
//         if (!ec) {
//             std::string request(buffer, length);
//             std::cout << "Received request:\n" << request << "\n";

//             std::string response = make_http_response("Hello world");
//             asio::write(socket, asio::buffer(response), ec);
//         }

//         socket.shutdown(tcp::socket::shutdown_both, ec);
//     } catch (std::exception& e) {
//         std::cerr << "Client error: " << e.what() << "\n";
//     }
// }

// void worker(asio::io_context& io_context, tcp::acceptor& acceptor) {
//     while (true) {
//         asio::error_code ec;
//         tcp::socket socket(io_context);
//         acceptor.accept(socket, ec);

//         if (!ec) {
//             handle_client(std::move(socket));
//         } else {
//             std::cerr << "Accept error: " << ec.message() << "\n";
//         }
//     }
// }

// int main() {
//     try {
//         const int num_threads = std::thread::hardware_concurrency();
//         asio::io_context io_context;

//         tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), 8080));
//         std::cout << "Server listening on port 8080 with " << num_threads << " threads...\n";

//         std::vector<std::thread> threads;
//         for (int i = 0; i < num_threads; ++i) {
//             threads.emplace_back(worker, std::ref(io_context), std::ref(acceptor));
//         }

//         for (;;)) {
//             t.join();
//         }

//     } catch (std::exception& e) {
//         std::cerr << "Server error: " << e.what() << "\n";
//     }

//     return 0;
// }

// The ASIO_STANDALONE define is necessary to use the standalone version of Asio.
// Remove if you are using Boost Asio.

#define ASIO_STANDALONE

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <iostream>
#include "utils/functions.hpp"

typedef websocketpp::server<websocketpp::config::asio> server;

namespace futils = fast_led_teleop::utils::functions;

class EchoServer {
public:
    EchoServer() {
        // Set message handler
        m_server.set_message_handler(
            [&](websocketpp::connection_hdl hdl, server::message_ptr msg) {
                m_server.send(hdl, msg->get_payload(), msg->get_opcode());
            });

        m_server.init_asio();
    }

    void run(uint16_t port) {
        m_server.listen(port);
        m_server.start_accept();
        m_server.run();
    }

private:
    server m_server;
};


int main() {
    auto addTwo = [](int x, int y){
        return x + y;
    };
    auto resultv1 = futils::callV1(4, 10, [](int arg1, int arg2) {
        return arg1-arg2;
    });
    std::cout<< resultv1 << std:: endl;

    auto resultv2 = futils::callV2(2, 20, [](int arg1, int arg2) {
        return arg1 * arg2;
    });
    std::cout << resultv2 << std::endl;

    return 0;
    // EchoServer server;
    // std::cout << "WebSocket echo server listening on port 9002..." << std::endl;
    // server.run(9002);
}