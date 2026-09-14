#pragma once

#include <SDL3/SDL.h>

#include <string>

namespace sdl {

[[nodiscard]] inline std::string last_error(const char* what) {
  const char* err = SDL_GetError();
  return std::string(what) + ": " + (err && *err ? err : "unknown error");
}

[[noreturn]] void throw_error(const char* what);

}  // namespace sdl
