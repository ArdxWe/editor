#include <SDL3/SDL_main.h>

#include <cstdlib>
#include <iostream>

#include "app.hpp"

int main(int argc, char* argv[]) {
  const bool prompt_folder = argc < 2;
  const char* path = prompt_folder ? "." : argv[1];
  try {
    sdl::App app{path, prompt_folder};
    app.run();
  } catch (const std::exception& ex) {
    std::cerr << ex.what() << '\n';
    return EXIT_FAILURE;
  }
}
