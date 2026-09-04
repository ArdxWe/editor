#pragma once

#include <SDL3/SDL.h>

namespace sdl {

class Window;
class Texture;

class Renderer {
public:
    Renderer() = default;
    explicit Renderer(const Window& window);
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    Renderer(Renderer&& other) noexcept;
    Renderer& operator=(Renderer&& other) noexcept;

    void set_draw_color(Uint8 r, Uint8 g, Uint8 b, Uint8 a) const;
    void clear() const;
    void copy(const Texture& texture, const SDL_FRect* src, const SDL_FRect* dst) const;
    void present() const;
    SDL_Renderer* get() const { return renderer_; }

private:
    SDL_Renderer* renderer_ = nullptr;
};

}  // namespace sdl
