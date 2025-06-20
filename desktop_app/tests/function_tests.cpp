#include <functional>
#include <gtest/gtest.h>
#include "utils/functions.hpp"

namespace futils = fast_led_teleop::utils::functions;

TEST(FunctionTests, Simple) {
    auto res1 = futils::callV1(1, 2, [](auto arg1, auto arg2) {
        return arg1 - arg2;
    });
    EXPECT_EQ(res1, -1);

    auto res2 = futils::callV2(4, 20, [](auto arg1, auto arg2) {
        return arg1 * arg2;
    }); 
    EXPECT_EQ(res2, 80);
}