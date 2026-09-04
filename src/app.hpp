#pragma once

#include "context.hpp"
#include "renderer.hpp"
#include "texture.hpp"
#include "window.hpp"

#include <filesystem>

namespace sdl {

class App {
public:
    explicit App(const std::filesystem::path& path);
    void run();

private:
    Context context_;
    Window window_;
    Renderer renderer_;
    Texture texture_;
};

}  // namespace sdl
