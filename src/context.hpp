#pragma once

#include "error.hpp"

#include <SDL3/SDL.h>

namespace sdl {

class Context {
public:
    explicit Context(SDL_InitFlags flags) {
        if (!SDL_Init(flags)) {
            throw_error("SDL_Init");
        }
    }

    ~Context() { SDL_Quit(); }

    Context(const Context&) = delete;
    Context& operator=(const Context&) = delete;
    Context(Context&&) = delete;
    Context& operator=(Context&&) = delete;
};

}  // namespace sdl
