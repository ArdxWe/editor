#include <SDL3/SDL_main.h>

#include <cstdlib>
#include <iostream>

#include "app.hpp"
#include "log.hpp"

int main(int argc, char* argv[]) {
  sdl::init_logging();
  const char* path = argc < 2 ? "." : argv[1];
  LOG_INFO("start path={}", path);

  try {
    sdl::App app{path};
    app.run();
    LOG_INFO("exit");
  } catch (const std::exception& ex) {
    LOG_ERROR("{}", ex.what());
    std::cerr << ex.what() << '\n';
    return EXIT_FAILURE;
  }
}
