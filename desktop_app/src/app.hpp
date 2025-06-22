#pragma once
#include <atomic>

namespace fast_led_teleop::desktop {
    int runApp(const std::atomic<bool>& stopSignal);
}