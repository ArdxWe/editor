#include "window.hpp"

#include "error.hpp"

namespace sdl {

Window::Window(const char* title, int width, int height, SDL_WindowFlags flags)
    : window_(SDL_CreateWindow(title, width, height, flags)) {
    if (!window_) {
        throw_error("SDL_CreateWindow");
    }
}

Window::~Window() {
    if (window_) {
        SDL_DestroyWindow(window_);
    }
}

Window::Window(Window&& other) noexcept : window_(other.window_) {
    other.window_ = nullptr;
}

Window& Window::operator=(Window&& other) noexcept {
    if (this != &other) {
        if (window_) {
            SDL_DestroyWindow(window_);
        }
        window_ = other.window_;
        other.window_ = nullptr;
    }
    return *this;
}

Window::Size Window::size() const {
    Size out;
    SDL_GetWindowSize(window_, &out.w, &out.h);
    return out;
}

void Window::lock_size(int width, int height) {
    SDL_SetWindowResizable(window_, false);
    SDL_SetWindowMinimumSize(window_, width, height);
    SDL_SetWindowMaximumSize(window_, width, height);
}

}  // namespace sdl
