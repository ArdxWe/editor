#pragma once

#include <SDL3/SDL.h>

namespace sdl {

class Window {
 public:
  Window() = default;
  Window(const char* title, int width, int height, SDL_WindowFlags flags);
  ~Window();

  Window(const Window&) = delete;
  Window& operator=(const Window&) = delete;
  Window(Window&& other) noexcept;
  Window& operator=(Window&& other) noexcept;

  struct Size {
    int w = 0;
    int h = 0;
  };

  Size size() const;
  void set_title(const char* title);
  void start_text_input();

  SDL_Window* get() const { return window_; }

 private:
  SDL_Window* window_ = nullptr;
};

}  // namespace sdl
