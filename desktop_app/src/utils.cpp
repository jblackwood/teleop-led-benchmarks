#include <functional>

namespace fast_led_teleop
{
    namespace utils
    {
        int callV2(int arg1, int arg2, std::function<int(int, int)> f)
        {
            return f(arg1, arg2);
        }
    }
}