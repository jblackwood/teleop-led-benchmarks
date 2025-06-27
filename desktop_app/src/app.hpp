#pragma once
#include <atomic>

namespace teleop_led_benchmarks
{
namespace desktop
{


enum class ConnectionType
{
    WebSocket,
    CustomTcp
};


int runApp(
    const std::atomic<bool>& stopSignal,
    const ConnectionType connType);


}  // namespace desktop
}  // namespace teleop_led_benchmarks