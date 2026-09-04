cmake_minimum_required(VERSION 3.24)
project(sdl_image_viewer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(APPLE AND EXISTS /opt/homebrew)
    list(APPEND CMAKE_PREFIX_PATH /opt/homebrew)
endif()

find_package(SDL3 REQUIRED CONFIG)
find_package(SDL3_image REQUIRED CONFIG)

add_executable(view_image
    src/main.cpp
    src/app.cpp
    src/surface.cpp
    src/window.cpp
    src/renderer.cpp
    src/texture.cpp
)
target_include_directories(view_image PRIVATE src)
target_link_libraries(view_image PRIVATE SDL3::SDL3 SDL3_image::SDL3_image)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(view_image PRIVATE -Wall -Wextra -Wpedantic)
endif()
