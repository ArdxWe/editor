#include "app.hpp"

#include "surface.hpp"

namespace sdl {

App::App(const std::filesystem::path& path) : context_(SDL_INIT_VIDEO) {
    Surface surface{path.string().c_str()};
    const int image_w = surface.width();
    const int image_h = surface.height();

    window_ = Window{
        path.filename().string().c_str(),
        image_w,
        image_h,
        SDL_WINDOW_HIGH_PIXEL_DENSITY};
    window_.lock_size(image_w, image_h);
    renderer_ = Renderer{window_};
    texture_ = Texture{renderer_, surface};
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

        renderer_.set_draw_color(18, 18, 20, 255);
        renderer_.clear();
        renderer_.copy(texture_, nullptr, nullptr);
        renderer_.present();
    }
}

}  // namespace sdl
