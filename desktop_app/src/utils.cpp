#include <functional>

namespace teleop_led_benchmarks
{
namespace utils
{


int callV2(int arg1, int arg2, std::function<int(int, int)> f)
{
    return f(arg1, arg2);
}


}  // namespace utils
}  // namespace teleop_led_benchmarks