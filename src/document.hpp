#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace sdl {

class Document {
 public:
  Document() = default;
  explicit Document(std::filesystem::path path);

  void load(std::filesystem::path path);
  void close();
  void save();

  void insert(const char* utf8);
  void backspace();
  void erase_forward();
  void newline();
  void move_left();
  void move_right();
  void move_up();
  void move_down();
  void move_home();
  void move_end();
  void click_column(int line, int byte_pos);

  const std::filesystem::path& path() const { return path_; }

  const std::vector<std::string>& lines() const { return lines_; }

  int row() const { return row_; }

  int col() const { return col_; }

  bool dirty() const { return dirty_; }

  bool has_file() const { return !path_.empty(); }

  bool can_edit() const { return can_edit_; }

  std::string title() const;

 private:
  void clamp();

  std::filesystem::path path_;
  std::vector<std::string> lines_{""};
  int row_ = 0;
  int col_ = 0;
  bool dirty_ = false;
  bool can_edit_ = false;
};

}  // namespace sdl
