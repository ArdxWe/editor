#include "font.hpp"

#include <SDL3/SDL.h>

#include <algorithm>
#include <filesystem>

#include "log.hpp"

namespace sdl {

namespace {

struct CjkFont {
  const char* path;
  Sint64 face;
};

const CjkFont cjk_font_candidates[] = {
    {"/System/Library/Fonts/PingFang.ttc", 0},
    {"/System/Library/Fonts/Hiragino Sans GB.ttc", 2},
    {"/System/Library/Fonts/Hiragino Sans GB.ttc", 0},
    {"/System/Library/Fonts/STHeiti Medium.ttc", 1},
    {"/System/Library/Fonts/STHeiti Light.ttc", 1},
    {"/System/Library/Fonts/Supplemental/Songti.ttc", 6},
    {"/System/Library/Fonts/Supplemental/Arial Unicode.ttf", 0},
    {"/Library/Fonts/Arial Unicode.ttf", 0},
};

TTF_Font* open_face(const char* path, float ptsize, Sint64 face) {
  if (!path || !std::filesystem::exists(path)) {
    return nullptr;
  }
  if (face <= 0) {
    return TTF_OpenFont(path, ptsize);
  }
  const SDL_PropertiesID props = SDL_CreateProperties();
  if (!props) {
    return nullptr;
  }
  SDL_SetStringProperty(props, TTF_PROP_FONT_CREATE_FILENAME_STRING, path);
  SDL_SetFloatProperty(props, TTF_PROP_FONT_CREATE_SIZE_FLOAT, ptsize);
  SDL_SetNumberProperty(props, TTF_PROP_FONT_CREATE_FACE_NUMBER, face);
  TTF_Font* font = TTF_OpenFontWithProperties(props);
  SDL_DestroyProperties(props);
  return font;
}

}  // namespace

void Font::attach_cjk_fallback(float ptsize) {
  for (const CjkFont& candidate : cjk_font_candidates) {
    TTF_Font* cjk = open_face(candidate.path, ptsize, candidate.face);
    if (!cjk) {
      continue;
    }
    TTF_SetFontHinting(cjk, TTF_HINTING_LIGHT);
    if (!TTF_AddFallbackFont(font_, cjk)) {
      TTF_CloseFont(cjk);
      continue;
    }
    fallback_ = cjk;
    const int skip =
        std::max(TTF_GetFontLineSkip(font_), TTF_GetFontLineSkip(cjk));
    TTF_SetFontLineSkip(font_, skip);
    LOG_DEBUG("cjk fallback {} face={}", candidate.path, candidate.face);
    return;
  }
}

Font::Font(const char* path, float ptsize) : font_(TTF_OpenFont(path, ptsize)) {
  if (!font_) {
    throw_error("TTF_OpenFont");
  }
  TTF_SetFontHinting(font_, TTF_HINTING_LIGHT);
  attach_cjk_fallback(ptsize);
}

Font::~Font() {
  if (font_) {
    TTF_ClearFallbackFonts(font_);
    TTF_CloseFont(font_);
  }
  if (fallback_) {
    TTF_CloseFont(fallback_);
  }
}

Font::Font(Font&& other) noexcept
    : font_(other.font_), fallback_(other.fallback_) {
  other.font_ = nullptr;
  other.fallback_ = nullptr;
}

Font& Font::operator=(Font&& other) noexcept {
  if (this != &other) {
    if (font_) {
      TTF_ClearFallbackFonts(font_);
      TTF_CloseFont(font_);
    }
    if (fallback_) {
      TTF_CloseFont(fallback_);
    }
    font_ = other.font_;
    fallback_ = other.fallback_;
    other.font_ = nullptr;
    other.fallback_ = nullptr;
  }
  return *this;
}

int Font::line_skip() const { return TTF_GetFontLineSkip(font_); }

int Font::measure(const char* text, std::size_t length) const {
  int w = 0;
  int h = 0;
  if (!text || !*text) {
    return 0;
  }
  TTF_GetStringSize(font_, text, length, &w, &h);
  return w;
}

std::size_t Font::fit(const char* text, int max_width,
                      int* measured_width) const {
  int width = 0;
  std::size_t bytes = 0;
  if (!text || !*text || max_width <= 0) {
    if (measured_width) {
      *measured_width = 0;
    }
    return 0;
  }
  TTF_MeasureString(font_, text, 0, max_width, &width, &bytes);
  if (measured_width) {
    *measured_width = width;
  }
  return bytes;
}

SDL_Surface* Font::render(const char* text, SDL_Color foreground,
                          SDL_Color background) const {
  if (!text || !*text) {
    return nullptr;
  }
  if (SDL_Surface* lcd =
          TTF_RenderText_LCD(font_, text, 0, foreground, background)) {
    return lcd;
  }
  return TTF_RenderText_Blended(font_, text, 0, foreground);
}

}  // namespace sdl
