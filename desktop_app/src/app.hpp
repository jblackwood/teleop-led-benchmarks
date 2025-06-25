#pragma once
#include <atomic>

namespace fast_led_teleop
{
    namespace desktop
    {
        int runApp(const std::atomic<bool>& stopSignal);
    }
}