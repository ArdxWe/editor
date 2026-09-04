#include "texture.hpp"

#include "error.hpp"
#include "renderer.hpp"
#include "surface.hpp"

namespace sdl {

Texture::Texture(const Renderer& renderer, const Surface& surface)
    : texture_(SDL_CreateTextureFromSurface(renderer.get(), surface.get())) {
    if (!texture_) {
        throw_error("SDL_CreateTextureFromSurface");
    }
}

Texture::~Texture() {
    if (texture_) {
        SDL_DestroyTexture(texture_);
    }
}

Texture::Texture(Texture&& other) noexcept : texture_(other.texture_) {
    other.texture_ = nullptr;
}

Texture& Texture::operator=(Texture&& other) noexcept {
    if (this != &other) {
        if (texture_) {
            SDL_DestroyTexture(texture_);
        }
        texture_ = other.texture_;
        other.texture_ = nullptr;
    }
    return *this;
}

void Texture::set_scale_mode(SDL_ScaleMode mode) const {
    SDL_SetTextureScaleMode(texture_, mode);
}

}  // namespace sdl
