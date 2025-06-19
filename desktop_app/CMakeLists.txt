cmake_minimum_required(VERSION 3.10)

project(MyApp)

add_executable(MyApp src/main.cpp)

find_package(glfw3 CONFIG REQUIRED)
target_link_libraries(MyApp PRIVATE glfw)
find_package(imgui CONFIG REQUIRED)
target_link_libraries(MyApp PRIVATE imgui::imgui)
find_package(Boost REQUIRED COMPONENTS beast)
target_link_libraries(MyApp PRIVATE Boost::beast)
find_package(OpenGL REQUIRED)
target_link_libraries(MyApp PRIVATE OpenGL::GL)
