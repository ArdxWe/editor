#include "app.hpp"

#include "surface.hpp"

#include <algorithm>

namespace sdl {

namespace {

SDL_FRect letterbox(float tex_w, float tex_h, float win_w, float win_h) {
    const float scale = std::min(win_w / tex_w, win_h / tex_h);
    const float w = tex_w * scale;
    const float h = tex_h * scale;
    return {(win_w - w) * 0.5f, (win_h - h) * 0.5f, w, h};
}

}  // namespace

App::App(const std::filesystem::path& path) : context_(SDL_INIT_VIDEO) {
    Surface surface{path.string().c_str()};
    const int image_w = surface.width();
    const int image_h = surface.height();

    window_ = Window{
        path.filename().string().c_str(),
        image_w,
        image_h,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY};
    renderer_ = Renderer{window_};
    texture_ = Texture{renderer_, surface};
    tex_w_ = static_cast<float>(image_w);
    tex_h_ = static_cast<float>(image_h);
    texture_.set_scale_mode(SDL_SCALEMODE_LINEAR);
}

void App::run() {
    bool running = true;
    while (running) {
        SDL_Event event{};
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            } else if (event.type == SDL_EVENT_KEY_DOWN &&
                       (event.key.key == SDLK_ESCAPE || event.key.key == SDLK_Q)) {
                running = false;
            }
        }

        const auto win = window_.size();
        const auto dest =
            letterbox(tex_w_, tex_h_, static_cast<float>(win.w), static_cast<float>(win.h));

        renderer_.set_draw_color(18, 18, 20, 255);
        renderer_.clear();
        renderer_.copy(texture_, nullptr, &dest);
        renderer_.present();
    }
}

}  // namespace sdl
