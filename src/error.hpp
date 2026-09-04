#pragma once

#include <SDL3/SDL.h>

#include <stdexcept>
#include <string>

namespace sdl {

[[nodiscard]] inline std::string last_error(const char* what) {
    const char* err = SDL_GetError();
    return std::string(what) + ": " + (err && *err ? err : "unknown error");
}

[[noreturn]] inline void throw_error(const char* what) {
    throw std::runtime_error(last_error(what));
}

}  // namespace sdl
