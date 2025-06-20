cmake_minimum_required(VERSION 3.10)

project(MyApp)

include_directories(include)

# app library
file(GLOB_RECURSE APP_SOURCES "src/*.cpp")
list(REMOVE_ITEM APP_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")
add_library(MyAppLib STATIC ${APP_SOURCES})
find_package(glfw3 CONFIG REQUIRED)
target_link_libraries(MyAppLib PRIVATE glfw)
find_package(imgui CONFIG REQUIRED)
target_link_libraries(MyAppLib PRIVATE imgui::imgui)
find_package(asio CONFIG REQUIRED)
target_link_libraries(MyAppLib PRIVATE asio::asio)
find_package(OpenGL REQUIRED)
target_link_libraries(MyAppLib PRIVATE OpenGL::GL)

# app executable
add_executable(MyApp "src/main.cpp")
target_link_libraries(MyApp PRIVATE MyAppLib)
target_link_libraries(MyApp PRIVATE asio::asio)

# test executable
file(GLOB_RECURSE TEST_SOURCES "tests/*.cpp")
add_executable(MyTest ${TEST_SOURCES})
target_link_libraries(MyTest PRIVATE MyAppLib)

# gtest
enable_testing()
find_package(GTest CONFIG REQUIRED)
target_link_libraries(MyTest PRIVATE GTest::gtest_main)
include(GoogleTest)
gtest_discover_tests(MyTest)