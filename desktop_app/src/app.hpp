#pragma once
#include <atomic>

namespace fast_led_teleop
{
    namespace desktop
    {
        enum class ConnectionType
        {
            WebSocket,
            CustomTcp
        };
        int runApp(const std::atomic<bool>& stopSignal, const ConnectionType connType);
    }
}