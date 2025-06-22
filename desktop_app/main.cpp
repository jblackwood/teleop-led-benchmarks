#include <iostream>
#include "app.hpp"
#include "utils.hpp"
#include <atomic>

namespace utils = fast_led_teleop::utils;

int main() {
    auto addTwo = [](int x, int y) { return x + y; };
    auto resultv1 =
        utils::callV1(4, 10, [](int arg1, int arg2) { return arg1 - arg2; });
    std::cout << resultv1 << std::endl;

    auto resultv2 =
        utils::callV2(2, 20, [](int arg1, int arg2) { return arg1 * arg2; });
    std::cout << resultv2 << std::endl;

    std::atomic<bool> stopFlag{false};
    fast_led_teleop::desktop::runApp(stopFlag);

    return 0;
}