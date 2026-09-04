#include "app.hpp"

#include <SDL3/SDL_main.h>

#include <cstdlib>
#include <iostream>

int main(int argc, char* argv[]) {
    const char* path = argc >= 2 ? argv[1] : ".";
    try {
        sdl::App app{path};
        app.run();
    } catch (const std::exception& ex) {
        std::cerr << ex.what() << '\n';
        return EXIT_FAILURE;
    }
}
