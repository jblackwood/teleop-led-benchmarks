#include <atomic>
#include <iostream>

#include "app.hpp"

namespace desktop = teleop_led_benchmarks::desktop;
using ConnectionType = desktop::ConnectionType;


int main(int argc, const char** argv)
{
    if (argc != 2)
    {
        std::cout << "Expected usage \"TeleopLed --[connectionType]\"" << '\n'
                  << "For example \"TeleopLed --websocket\"" << '\n'
                  << "Supported connection types are websocket, customTcp, and ..." << std::endl;
        return 0;
    }
    const std::string connStr = argv[1];
    ConnectionType connType;
    if (connStr == "--websocket")
    {
        connType = ConnectionType::WebSocket;
    }
    else if (connStr == "--customTcp")
    {
        connType = ConnectionType::CustomTcp;
    }
    else
    {
        std::cerr << "Unknown connection type: " << connStr << std::endl;
        return 1;
    }

    std::atomic<bool> stopFlag{false};
    desktop::runApp(stopFlag, connType);
    return 0;
}