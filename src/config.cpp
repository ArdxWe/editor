#include "config.hpp"

#include <SDL3/SDL.h>

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <string_view>

#include "log.hpp"

namespace sdl {

namespace {

std::string trim(std::string_view s) {
  const auto begin = s.find_first_not_of(" \t\r\n");
  if (begin == std::string_view::npos) {
    return {};
  }
  const auto end = s.find_last_not_of(" \t\r\n");
  return std::string{s.substr(begin, end - begin + 1)};
}

void apply_line(Config& cfg, std::string_view line) {
  const auto comment = line.find('#');
  if (comment != std::string_view::npos) {
    line = line.substr(0, comment);
  }
  line = trim(line);
  if (line.empty()) {
    return;
  }
  const auto eq = line.find('=');
  if (eq == std::string_view::npos) {
    return;
  }
  const std::string key = trim(line.substr(0, eq));
  std::string value = trim(line.substr(eq + 1));
  if (value.size() >= 2 && ((value.front() == '"' && value.back() == '"') ||
                            (value.front() == '\'' && value.back() == '\''))) {
    value = value.substr(1, value.size() - 2);
  }
  if (key == "font" && !value.empty()) {
    cfg.font = std::move(value);
  }
}

bool load_file(const std::filesystem::path& path, Config& cfg) {
  std::ifstream in{path};
  if (!in) {
    return false;
  }
  std::string line;
  while (std::getline(in, line)) {
    apply_line(cfg, line);
  }
  LOG_INFO("config {}", path.string());
  return true;
}

}  // namespace

Config load_config() {
  Config cfg;
  if (load_file("editor.conf", cfg)) {
    return cfg;
  }
  if (const char* base = SDL_GetBasePath()) {
    load_file(std::filesystem::path(base) / "editor.conf", cfg);
  }
  return cfg;
}

std::string resolve_font(const std::string& path) {
  std::filesystem::path p{path};
  if (!p.is_absolute()) {
    if (const char* base = SDL_GetBasePath()) {
      p = std::filesystem::path(base) / p;
    }
  }
  p = p.lexically_normal();
  if (!std::filesystem::exists(p)) {
    throw std::runtime_error("font not found: " + p.string());
  }
  LOG_DEBUG("font {}", p.string());
  return p.string();
}

}  // namespace sdl
