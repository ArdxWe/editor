#pragma once

#include <SDL3/SDL.h>

namespace sdl {

class Surface {
 public:
  explicit Surface(SDL_Surface* raw);
  ~Surface();

  Surface(const Surface&) = delete;
  Surface& operator=(const Surface&) = delete;
  Surface(Surface&& other) noexcept;
  Surface& operator=(Surface&& other) noexcept;

  int width() const;
  int height() const;

  SDL_Surface* get() const { return surface_; }

 private:
  SDL_Surface* surface_ = nullptr;
};

}  // namespace sdl
