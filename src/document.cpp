#include "document.hpp"

#include <fstream>
#include <stdexcept>

#include "utf8.hpp"

namespace sdl {

namespace {

constexpr std::uintmax_t kMaxBytes = 32 * 1024 * 1024;  // 32 MB

std::string strip_cr(std::string line) {
  if (!line.empty() && line.back() == '\r') {
    line.pop_back();
  }
  return line;
}

}  // namespace

Document::Document(std::filesystem::path path) { load(std::move(path)); }

void Document::load(std::filesystem::path path) {
  path_ = std::move(path);
  lines_.assign(1, "");
  row_index_ = 0;
  line_offset_ = 0;
  dirty_ = false;
  can_edit_ = true;

  std::error_code ec;
  const auto nbytes = std::filesystem::file_size(path_, ec);
  if (!ec && nbytes > kMaxBytes) {
    lines_[0] = "[file too large]";
    can_edit_ = false;
    return;
  }

  std::ifstream in{path_, std::ios::binary};
  if (!in) {
    return;  // Missing file: keep an empty editable buffer at this path.
  }

  // Stream one line at a time so we do not hold a second copy of the file.
  lines_.clear();
  std::string line;
  std::uintmax_t read = 0;

  while (std::getline(in, line)) {
    read += line.size() + 1;
    if (read > kMaxBytes || line.find('\0') != std::string::npos) {
      lines_.assign(1, read > kMaxBytes ? "[file too large]" : "[binary file]");
      can_edit_ = false;
      return;
    }
    lines_.push_back(strip_cr(std::move(line)));
  }
  if (lines_.empty()) {
    lines_.emplace_back();
  } else if (nbytes > 0) {
    // getline discards the delimiter; keep a trailing empty line if the file
    // ended with '\n'.
    in.clear();
    in.seekg(-1, std::ios::end);
    char ch = 0;
    if (in.get(ch) && ch == '\n') {
      lines_.emplace_back();
    }
  }
}

void Document::close() {
  path_.clear();
  lines_.assign(1, "");
  row_index_ = 0;
  line_offset_ = 0;
  dirty_ = false;
  can_edit_ = false;
}

void Document::save() {
  if (!has_file() || !can_edit_) {
    return;
  }
  std::ofstream out{path_, std::ios::binary};
  if (!out) {
    throw std::runtime_error("failed to save " + path_.string());
  }
  for (std::size_t i = 0; i < lines_.size(); ++i) {
    out << lines_[i];
    if (i + 1 < lines_.size()) {
      out << '\n';
    }
  }
  dirty_ = false;
}

void Document::insert(const char* utf8) {
  if (!utf8 || !*utf8) {
    return;
  }
  auto& line = lines_[static_cast<std::size_t>(row_index_)];
  const std::string text{utf8};
  line.insert(static_cast<std::size_t>(line_offset_), text);
  line_offset_ += static_cast<int>(
      text.size());  // Advance by bytes, matching line_offset_.
  dirty_ = true;
}

void Document::backspace() {
  auto& line = lines_[static_cast<std::size_t>(row_index_)];
  if (line_offset_ > 0) {
    const int prev = static_cast<int>(
        utf8_prev(line, static_cast<std::size_t>(line_offset_)));
    line.erase(static_cast<std::size_t>(prev),
               static_cast<std::size_t>(line_offset_ - prev));
    line_offset_ = prev;
    dirty_ = true;
    return;
  }
  // At column 0: join this line onto the previous one.
  if (row_index_ == 0) {
    return;
  }
  auto& prev_line = lines_[static_cast<std::size_t>(row_index_ - 1)];
  line_offset_ = static_cast<int>(prev_line.size());
  prev_line += line;
  lines_.erase(lines_.begin() + row_index_);
  --row_index_;
  dirty_ = true;
}

void Document::erase_forward() {
  auto& line = lines_[static_cast<std::size_t>(row_index_)];
  if (line_offset_ < static_cast<int>(line.size())) {
    const int next = static_cast<int>(
        utf8_next(line, static_cast<std::size_t>(line_offset_)));
    line.erase(static_cast<std::size_t>(line_offset_),
               static_cast<std::size_t>(next - line_offset_));
    dirty_ = true;
    return;
  }
  // At end of line: pull the next line up.
  if (row_index_ + 1 >= static_cast<int>(lines_.size())) {
    return;
  }
  line += lines_[static_cast<std::size_t>(row_index_ + 1)];
  lines_.erase(lines_.begin() + row_index_ + 1);
  dirty_ = true;
}

void Document::newline() {
  auto& line = lines_[static_cast<std::size_t>(row_index_)];
  std::string rest = line.substr(static_cast<std::size_t>(line_offset_));
  line.resize(static_cast<std::size_t>(line_offset_));
  lines_.insert(lines_.begin() + row_index_ + 1, std::move(rest));
  ++row_index_;
  line_offset_ = 0;
  dirty_ = true;
}

void Document::move_left() {
  if (line_offset_ > 0) {
    line_offset_ =
        static_cast<int>(utf8_prev(lines_[static_cast<std::size_t>(row_index_)],
                                   static_cast<std::size_t>(line_offset_)));
    return;
  }
  if (row_index_ > 0) {
    --row_index_;
    line_offset_ =
        static_cast<int>(lines_[static_cast<std::size_t>(row_index_)].size());
  }
}

void Document::move_right() {
  auto& line = lines_[static_cast<std::size_t>(row_index_)];
  if (line_offset_ < static_cast<int>(line.size())) {
    line_offset_ = static_cast<int>(
        utf8_next(line, static_cast<std::size_t>(line_offset_)));
    return;
  }
  if (row_index_ + 1 < static_cast<int>(lines_.size())) {
    ++row_index_;
    line_offset_ = 0;
  }
}

void Document::move_up() {
  if (row_index_ == 0) {
    return;
  }
  --row_index_;
  clamp();  // Same visual column is not tracked; stay within the new line.
}

void Document::move_down() {
  if (row_index_ + 1 >= static_cast<int>(lines_.size())) {
    return;
  }
  ++row_index_;
  clamp();
}

void Document::move_home() { line_offset_ = 0; }

void Document::move_end() {
  line_offset_ =
      static_cast<int>(lines_[static_cast<std::size_t>(row_index_)].size());
}

void Document::click_column(int line, int byte_pos) {
  row_index_ = line;
  if (row_index_ < 0) {
    row_index_ = 0;
  }
  if (row_index_ >= static_cast<int>(lines_.size())) {
    row_index_ = static_cast<int>(lines_.size()) - 1;
  }
  line_offset_ = byte_pos;
  clamp();
}

std::string Document::title() const {
  std::string name = path_.filename().string();
  if (name.empty()) {
    name = path_.string();
  }
  if (dirty_) {
    name += " *";
  }
  return name;
}

void Document::clamp() {
  auto& line = lines_[static_cast<std::size_t>(row_index_)];
  if (line_offset_ > static_cast<int>(line.size())) {
    line_offset_ = static_cast<int>(line.size());
  }
  if (line_offset_ < 0) {
    line_offset_ = 0;
  }
}

}  // namespace sdl
