#include "renderer.hpp"

#include "error.hpp"
#include "texture.hpp"
#include "window.hpp"

namespace sdl {

Renderer::Renderer(const Window& window)
    : renderer_(SDL_CreateRenderer(window.get(), nullptr)) {
  if (!renderer_) {
    throw_error("SDL_CreateRenderer");
  }
}

Renderer::~Renderer() {
  if (renderer_) {
    SDL_DestroyRenderer(renderer_);
  }
}

Renderer::Renderer(Renderer&& other) noexcept : renderer_(other.renderer_) {
  other.renderer_ = nullptr;
}

Renderer& Renderer::operator=(Renderer&& other) noexcept {
  if (this != &other) {
    if (renderer_) {
      SDL_DestroyRenderer(renderer_);
    }
    renderer_ = other.renderer_;
    other.renderer_ = nullptr;
  }
  return *this;
}

void Renderer::set_draw_color(Uint8 red, Uint8 green, Uint8 blue,
                              Uint8 alpha) const {
  SDL_SetRenderDrawColor(renderer_, red, green, blue, alpha);
}

void Renderer::clear() const { SDL_RenderClear(renderer_); }

void Renderer::fill_rect(const SDL_FRect& rect) const {
  SDL_RenderFillRect(renderer_, &rect);
}

void Renderer::set_clip(const SDL_Rect* rect) const {
  SDL_SetRenderClipRect(renderer_, rect);
}

void Renderer::copy(const Texture& texture, const SDL_FRect* src,
                    const SDL_FRect* dst) const {
  SDL_RenderTexture(renderer_, texture.get(), src, dst);
}

void Renderer::present() const { SDL_RenderPresent(renderer_); }

// 1 locks present() to the display refresh (typically 60Hz) to avoid tearing.
void Renderer::set_vsync(int vsync) const {
  SDL_SetRenderVSync(renderer_, vsync);
}

Renderer::Size Renderer::output_size() const {
  Size out;
  SDL_GetRenderOutputSize(renderer_, &out.w, &out.h);
  return out;
}

}  // namespace sdl
