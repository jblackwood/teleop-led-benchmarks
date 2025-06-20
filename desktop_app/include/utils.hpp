#pragma once
#include <functional>

namespace fast_led_teleop::utils {
template <typename F> int callV1(int arg1, int arg2, F f) {
    return f(arg1, arg2);
}

int callV2(int arg1, int arg2, std::function<int(int, int)> f);
} // namespace fast_led_teleop::utils