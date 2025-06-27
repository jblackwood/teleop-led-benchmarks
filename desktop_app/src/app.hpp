#pragma once
#include <atomic>

namespace teleop_led_benchmarks
{
namespace desktop
{


enum class ConnectionType
{
    WEB_SOCKET,
    CUSTOM_TCP
};


int runApp(
    const std::atomic<bool>& stopSignal,
    const ConnectionType connType);


}  // namespace desktop
}  // namespace teleop_led_benchmarks