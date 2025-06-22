#include <gtest/gtest.h>
#include "app.hpp"
#include <future>
#include <thread>
#include <iostream>

namespace fast_led_teleop::tests {
    namespace desktop = fast_led_teleop::desktop;

    TEST(AppTest, StopApp) {
        std::atomic<bool> stopFlag{false};
        // auto futExitCode = std::async(desktop::runApp, std::ref(stopFlag));
        auto futExitCode = std::async([&](){return desktop::runApp(stopFlag);});
        std::cout << "App started" << std::endl; 
        std::this_thread::sleep_for(std::chrono::seconds(2));
        stopFlag.store(true, std::memory_order_relaxed);
        auto exitCode = futExitCode.get();
        ASSERT_EQ(exitCode, 0);
    }
}