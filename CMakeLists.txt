cmake_minimum_required(VERSION 3.20)
project(sdl_text_editor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(SDL3 REQUIRED CONFIG)
find_package(SDL3_ttf REQUIRED CONFIG)
find_package(spdlog REQUIRED CONFIG)

file(GLOB editor_src CONFIGURE_DEPENDS src/*.cpp)
add_executable(editor ${editor_src})
target_include_directories(editor PRIVATE src)
target_link_libraries(editor PRIVATE spdlog::spdlog SDL3_ttf::SDL3_ttf SDL3::SDL3)
target_compile_options(editor PRIVATE -Wall -Wextra -Wpedantic)

add_custom_command(TARGET editor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/editor.conf
        $<TARGET_FILE_DIR:editor>/editor.conf)
