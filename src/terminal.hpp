#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace sdl {

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

  const std::vector<std::string>& lines() const { return lines_; }
  int cursor_row() const { return row_; }
  int cursor_col() const { return col_; }

 private:
  void feed(const char* data, std::size_t size);
  void put_utf8(std::string_view ch);
  void newline();
  void ensure_line();
  void erase_to_end();

  int master_ = -1;
  long pid_ = -1;
  int cols_ = 80;
  int rows_ = 24;
  int row_ = 0;
  int col_ = 0;
  int parse_ = 0;
  std::string pending_;
  std::vector<std::string> lines_{""};
};

}  // namespace sdl
