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
  if (window_ != nullptr) {
    SDL_DestroyWindow(window_);
  }
}

Window::Window(Window&& other) noexcept : window_(other.window_) {
  other.window_ = nullptr;
}

Window& Window::operator=(Window&& other) noexcept {
  if (this != &other) {
    if (window_ != nullptr) {
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

void Window::set_title(const char* title) {
  SDL_SetWindowTitle(window_, title);
}

void Window::show() {
  SDL_ShowWindow(window_);
  SDL_RaiseWindow(window_);
}

void Window::start_text_input() {
  if (!SDL_StartTextInput(window_)) {
    throw_error("SDL_StartTextInput");
  }
}

}  // namespace sdl
