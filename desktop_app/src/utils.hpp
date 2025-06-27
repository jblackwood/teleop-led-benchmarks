#pragma once
#include <functional>

namespace teleop_led_benchmarks
{
namespace utils
{


template <typename F>
int callV1(int arg1, int arg2, F f)
{
    return f(arg1, arg2);
}


int callV2(int arg1, int arg2, std::function<int(int, int)> f);


}  // namespace utils
}  // namespace teleop_led_benchmarks