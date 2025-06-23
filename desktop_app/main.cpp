#include <iostream>
#include "app.hpp"
#include "utils.hpp"
#include <atomic>

namespace utils = fast_led_teleop::utils;

int main() {
    std::atomic<bool> stopFlag{false};
    fast_led_teleop::desktop::runApp(stopFlag);
    return 0;
}