#include <SDL3/SDL_main.h>

#include <cstdlib>
#include <iostream>

#include "app.hpp"
#include "log.hpp"

int main(int argc, char* argv[]) {
  sdl::init_logging();
  const bool prompt_folder = argc < 2;
  const char* path = prompt_folder ? "." : argv[1];
  LOG_INFO("start path={} prompt_folder={}", path, prompt_folder);

  try {
    sdl::App app{path, prompt_folder};
    app.run();
    LOG_INFO("exit");
  } catch (const std::exception& ex) {
    LOG_ERROR("{}", ex.what());
    std::cerr << ex.what() << '\n';
    return EXIT_FAILURE;
  }
}
