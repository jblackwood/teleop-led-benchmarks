#include "app.hpp"
#include <gtest/gtest.h>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <future>
#include <iostream>
#include <thread>

namespace teleop_led_benchmarks::tests {
    namespace desktop = teleop_led_benchmarks::desktop;
    namespace beast = boost::beast;
    namespace http = beast::http;
    namespace websocket = beast::websocket;
    namespace asio = boost::asio;
    using tcp = boost::asio::ip::tcp;
    using ConnectionType = desktop::ConnectionType;

    TEST(AppTest, StopApp) {
        std::atomic<bool> stopFlag{false};
        auto futExitCode = std::async([&stopFlag]() { return desktop::runApp(stopFlag, ConnectionType::WebSocket); });
        std::this_thread::sleep_for(std::chrono::seconds(2));
        stopFlag.store(true, std::memory_order_relaxed);
        auto exitCode = futExitCode.get();
        ASSERT_EQ(exitCode, 0);
    }

    TEST(AppTest, ConnectWebsocket) {
        std::atomic<bool> stopFlag{false};
        auto futExitCode = std::async([&stopFlag]() { return desktop::runApp(stopFlag, ConnectionType::WebSocket); });
        std::this_thread::sleep_for(std::chrono::seconds(2));
        asio::io_context ioc{};
        tcp::resolver resolver{ioc};
        websocket::stream<tcp::socket> ws{ioc};
        std::string host = "127.0.0.1";

        // Look up the domain name
        auto const results = resolver.resolve(host, "9002");

        std::cout << "Trying to connect ws client" << std::endl;
        // Make the connection on the IP address we get from a lookup
        auto ep = asio::connect(ws.next_layer(), results);

        // Update the host_ string. This will provide the value of the
        // Host HTTP header during the WebSocket handshake.
        // See https://tools.ietf.org/html/rfc7230#section-5.4
        host += ":" + std::to_string(ep.port());

        // Set a decorator to change the User-Agent of the handshake
        ws.set_option(websocket::stream_base::decorator(
            [](websocket::request_type& req) {
                req.set(http::field::user_agent,
                        std::string(BOOST_BEAST_VERSION_STRING) +
                            " websocket-client-coro");
            }));

        // Perform the websocket handshake
        ws.handshake(host, "/");

        // Sleep for a bit
        std::this_thread::sleep_for(std::chrono::seconds(2));

        // Close the WebSocket connection
        ws.close(websocket::close_code::normal);

        // Send the message
        // ws.write(net::buffer(std::string(text)));

        stopFlag.store(true, std::memory_order_relaxed);
        auto exitCode = futExitCode.get();
        ASSERT_EQ(exitCode, 0);
    }
}