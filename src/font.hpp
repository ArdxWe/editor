#pragma once

#include <SDL3_ttf/SDL_ttf.h>

#include <cstddef>

#include "error.hpp"

namespace sdl {

class TtfContext {
 public:
  TtfContext() {
    if (!TTF_Init()) {
      throw_error("TTF_Init");
    }
  }

  ~TtfContext() { TTF_Quit(); }

  TtfContext(const TtfContext&) = delete;
  TtfContext& operator=(const TtfContext&) = delete;
};

class Font {
 public:
  Font() = default;
  Font(const char* path, float ptsize);
  ~Font();

  Font(const Font&) = delete;
  Font& operator=(const Font&) = delete;
  Font(Font&& other) noexcept;
  Font& operator=(Font&& other) noexcept;

  int line_skip() const;
  int measure(const char* text, std::size_t length = 0) const;
  std::size_t fit(const char* text, int max_width,
                  int* measured_width = nullptr) const;
  SDL_Surface* render(const char* text, SDL_Color foreground,
                      SDL_Color background) const;

  TTF_Font* get() const { return font_; }

 private:
  void attach_cjk_fallback(float ptsize);

  TTF_Font* font_ = nullptr;
  TTF_Font* fallback_ = nullptr;
};

}  // namespace sdl
