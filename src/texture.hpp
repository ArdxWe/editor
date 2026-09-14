#pragma once

#include <SDL3/SDL.h>

namespace sdl {

class Renderer;
class Surface;

class Texture {
 public:
  Texture() = default;
  Texture(const Renderer& renderer, const Surface& surface);
  ~Texture();

  Texture(const Texture&) = delete;
  Texture& operator=(const Texture&) = delete;
  Texture(Texture&& other) noexcept;
  Texture& operator=(Texture&& other) noexcept;

  float width() const;
  float height() const;

  SDL_Texture* get() const { return texture_; }

 private:
  SDL_Texture* texture_ = nullptr;
};

}  // namespace sdl
