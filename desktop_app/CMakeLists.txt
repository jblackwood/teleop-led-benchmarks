cmake_minimum_required(VERSION 3.10)

project(MyApp)

include_directories(
    src
    third_party
)

# app library
file(GLOB_RECURSE APP_SOURCES "src/*.cpp")
add_library(MyAppLib STATIC ${APP_SOURCES})
find_package(glfw3 CONFIG REQUIRED)
target_link_libraries(MyAppLib PRIVATE glfw)
find_package(imgui CONFIG REQUIRED)
target_link_libraries(MyAppLib PRIVATE imgui::imgui)
find_package(boost_beast CONFIG REQUIRED)
target_link_libraries(MyAppLib PRIVATE Boost::beast)
find_package(OpenGL REQUIRED)
target_link_libraries(MyAppLib PRIVATE OpenGL::GL)
find_package(nlohmann_json CONFIG REQUIRED)
target_link_libraries(MyAppLib PRIVATE nlohmann_json::nlohmann_json)
set(nlohmann-json_IMPLICIT_CONVERSIONS OFF)

# app executable
add_executable(MyApp "main.cpp")
target_link_libraries(MyApp PRIVATE MyAppLib)
# target_link_libraries(MyApp PRIVATE Boost::beast)

# test executable
file(GLOB_RECURSE TEST_SOURCES "tests/*.cpp")
add_executable(MyTest ${TEST_SOURCES})
target_link_libraries(MyTest PRIVATE MyAppLib)
target_link_libraries(MyTest PRIVATE Boost::beast)

# gtest
enable_testing()
find_package(GTest CONFIG REQUIRED)
target_link_libraries(MyTest PRIVATE GTest::gtest_main)
include(GoogleTest)
gtest_discover_tests(MyTest)

# compiler warnings
set(WARNING_FLAGS -Wall -Wextra -Werror)
target_compile_options(MyAppLib PRIVATE ${WARNING_FLAGS})
target_compile_options(MyApp PRIVATE ${WARNING_FLAGS})
target_compile_options(MyTest PRIVATE ${WARNING_FLAGS})