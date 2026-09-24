#pragma once

#include <SDL3/SDL.h>

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace sdl {

inline constexpr SDL_Color kTermFg{48, 46, 42, 255};
inline constexpr SDL_Color kTermBg{236, 234, 228, 255};

// One screen cell: a UTF-8 glyph plus the colors from the active SGR pen.
// Lines are vectors of these so zsh/ls colors can be drawn run-by-run.
struct TermCell {
  std::string ch;             // Usually one code point; may be multi-byte UTF-8
  SDL_Color fg = kTermFg;     // Foreground (text)
  SDL_Color bg = kTermBg;     // Background (drawn when not the default)
};

class Terminal {
 public:
  Terminal() = default;
  ~Terminal();

  Terminal(const Terminal&) = delete;
  Terminal& operator=(const Terminal&) = delete;
  Terminal(Terminal&&) = delete;
  Terminal& operator=(Terminal&&) = delete;

  bool start(const std::filesystem::path& cwd);
  void stop();

  bool running() const { return master_ >= 0; }

  void write(const char* data, std::size_t size);

  void write(const std::string& text) { write(text.data(), text.size()); }

  bool poll();
  void resize(int cols, int rows);

  const std::vector<std::vector<TermCell>>& lines() const { return lines_; }

  int cursor_row() const { return row_; }

  int cursor_col() const { return col_; }

 private:
  void feed(const char* data, std::size_t size);
  void put_utf8(std::string_view ch);
  void newline();
  void ensure_line();
  void erase_to_end();
  void erase_display();
  void apply_csi(char final);
  void apply_sgr(const std::vector<int>& params);
  void reset_pen();
  SDL_Color paint_fg() const;
  SDL_Color paint_bg() const;

  int master_ = -1;
  long pid_ = -1;
  int cols_ = 80;
  int rows_ = 24;
  int row_ = 0;
  int col_ = 0;
  int parse_ = 0;
  std::string pending_;
  std::string csi_;
  SDL_Color pen_fg_ = kTermFg;
  SDL_Color pen_bg_ = kTermBg;
  bool bold_ = false;
  bool inverse_ = false;
  std::vector<std::vector<TermCell>> lines_{{}};
};

}  // namespace sdl
