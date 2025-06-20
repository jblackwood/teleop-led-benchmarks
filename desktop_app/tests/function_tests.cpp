#include <functional>
#include <gtest/gtest.h>
#include "fast_led_teleop/utils.hpp"

namespace utils = fast_led_teleop::utils;

TEST(FunctionTests, Simple) {
    auto res1 = utils::callV1(1, 2, [](auto arg1, auto arg2) {
        return arg1 - arg2;
    });
    EXPECT_EQ(res1, -1);

    auto res2 = utils::callV2(4, 20, [](auto arg1, auto arg2) {
        return arg1 * arg2;
    }); 
    EXPECT_EQ(res2, 80);
}