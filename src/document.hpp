#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace sdl {

// In-memory buffer of UTF-8 lines. `line_offset_` is a byte offset, not a glyph index.
class Document {
 public:
  Document() = default;
  explicit Document(std::filesystem::path path);

  void load(std::filesystem::path path);  // Reads the file into in-memory lines_
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
  void click_column(int line, int byte_pos);  // Clicked byte, then clamp()

  const std::filesystem::path& path() const { return path_; }

  const std::vector<std::string>& lines() const { return lines_; }

  int row() const { return row_index_; }

  int line_offset() const { return line_offset_; }

  bool dirty() const { return dirty_; }

  bool has_file() const { return !path_.empty(); }

  bool can_edit() const { return can_edit_; }  // False for binary files

  std::string title() const;

 private:
  void clamp();  // Keep line_offset_ within the current line

  std::filesystem::path path_;          // Empty means no file is open
  std::vector<std::string> lines_{""};  // Always at least one (possibly empty) line
  int row_index_ = 0;                   // Current line index into lines_
  int line_offset_ = 0;                 // Byte offset into lines_[row_index_]
  bool dirty_ = false;                  // True if unsaved edits
  bool can_edit_ = false;               // False for binary or when no file is loaded
};

}  // namespace sdl
