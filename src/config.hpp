#pragma once

#include <string>

namespace sdl {

struct Config {
  // Relative to the build/ binary directory unless absolute.
  std::string font = "../assets/fonts/jetbrains-mono-regular.ttf";
};

Config load_config();
std::string resolve_font(const std::string& path);

}  // namespace sdl
