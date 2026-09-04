#include "surface.hpp"

#include "error.hpp"

#include <SDL3_image/SDL_image.h>

namespace sdl {

Surface::Surface(const char* path) : surface_(IMG_Load(path)) {
    if (!surface_) {
        throw_error("IMG_Load");
    }
}

Surface::~Surface() {
    if (surface_) {
        SDL_DestroySurface(surface_);
    }
}

Surface::Surface(Surface&& other) noexcept : surface_(other.surface_) {
    other.surface_ = nullptr;
}

Surface& Surface::operator=(Surface&& other) noexcept {
    if (this != &other) {
        if (surface_) {
            SDL_DestroySurface(surface_);
        }
        surface_ = other.surface_;
        other.surface_ = nullptr;
    }
    return *this;
}

int Surface::width() const { return surface_->w; }

int Surface::height() const { return surface_->h; }

}  // namespace sdl
