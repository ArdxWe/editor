#pragma once

#include <SDL3/SDL.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace sdl {

enum class Language {
  None,
  Cpp,
  Python,
  JavaScript,
  Json,
  Rust,
  Go,
  CMake,
  Shell,
  Markdown
};

enum class TokenKind {
  Text,
  Keyword,
  Constant,
  String,
  Escape,
  Comment,
  Number,
  Type,
  Function,
  Property,
  Macro,
  Operator
};

struct Token {
  std::size_t begin = 0;
  std::size_t end = 0;
  TokenKind kind = TokenKind::Text;
};

Language language_from_path(std::string_view path);
SDL_Color token_color(TokenKind kind);

class Highlighter {
 public:
  void set_language(Language lang);
  void invalidate(int row);
  std::vector<Token> tokens(int row, const std::string& line,
                            const std::vector<std::string>& lines);

 private:
  void resync(const std::vector<std::string>& lines);

  Language lang_ = Language::None;
  std::vector<std::uint8_t> state_before_;
  int dirty_from_ = 0;
};

}  // namespace sdl
