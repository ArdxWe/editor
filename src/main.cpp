#include "app.hpp"

#include <SDL3/SDL_main.h>

#include <cstdlib>
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <image>\n";
        return EXIT_FAILURE;
    }

    try {
        sdl::App app{argv[1]};
        app.run();
    } catch (const std::exception& ex) {
        std::cerr << ex.what() << '\n';
        return EXIT_FAILURE;
    }
}
