#include "document.hpp"

#include <fstream>
#include <iterator>
#include <stdexcept>

#include "utf8.hpp"

namespace sdl {

namespace {

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
  row_ = 0;
  col_ = 0;
  dirty_ = false;
  can_edit_ = true;

  std::ifstream in{path_, std::ios::binary};
  if (!in) {
    return;
  }
  const std::string all{(std::istreambuf_iterator<char>(in)),
                        std::istreambuf_iterator<char>()};
  if (all.find('\0') != std::string::npos) {
    lines_[0] = "[binary file]";
    can_edit_ = false;
    return;
  }

  lines_.clear();
  std::size_t start = 0;
  while (start <= all.size()) {
    const std::size_t end = all.find('\n', start);
    if (end == std::string::npos) {
      lines_.push_back(strip_cr(all.substr(start)));
      break;
    }
    lines_.push_back(strip_cr(all.substr(start, end - start)));
    start = end + 1;
    if (start == all.size()) {
      lines_.emplace_back();
      break;
    }
  }
  if (lines_.empty()) {
    lines_.emplace_back();
  }
}

void Document::close() {
  path_.clear();
  lines_.assign(1, "");
  row_ = 0;
  col_ = 0;
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
  auto& line = lines_[static_cast<std::size_t>(row_)];
  const std::string text{utf8};
  line.insert(static_cast<std::size_t>(col_), text);
  col_ += static_cast<int>(text.size());
  dirty_ = true;
}

void Document::backspace() {
  auto& line = lines_[static_cast<std::size_t>(row_)];
  if (col_ > 0) {
    const int prev =
        static_cast<int>(utf8_prev(line, static_cast<std::size_t>(col_)));
    line.erase(static_cast<std::size_t>(prev),
               static_cast<std::size_t>(col_ - prev));
    col_ = prev;
    dirty_ = true;
    return;
  }
  if (row_ == 0) {
    return;
  }
  auto& prev_line = lines_[static_cast<std::size_t>(row_ - 1)];
  col_ = static_cast<int>(prev_line.size());
  prev_line += line;
  lines_.erase(lines_.begin() + row_);
  --row_;
  dirty_ = true;
}

void Document::erase_forward() {
  auto& line = lines_[static_cast<std::size_t>(row_)];
  if (col_ < static_cast<int>(line.size())) {
    const int next =
        static_cast<int>(utf8_next(line, static_cast<std::size_t>(col_)));
    line.erase(static_cast<std::size_t>(col_),
               static_cast<std::size_t>(next - col_));
    dirty_ = true;
    return;
  }
  if (row_ + 1 >= static_cast<int>(lines_.size())) {
    return;
  }
  line += lines_[static_cast<std::size_t>(row_ + 1)];
  lines_.erase(lines_.begin() + row_ + 1);
  dirty_ = true;
}

void Document::newline() {
  auto& line = lines_[static_cast<std::size_t>(row_)];
  std::string rest = line.substr(static_cast<std::size_t>(col_));
  line.resize(static_cast<std::size_t>(col_));
  lines_.insert(lines_.begin() + row_ + 1, std::move(rest));
  ++row_;
  col_ = 0;
  dirty_ = true;
}

void Document::move_left() {
  if (col_ > 0) {
    col_ = static_cast<int>(utf8_prev(lines_[static_cast<std::size_t>(row_)],
                                      static_cast<std::size_t>(col_)));
    return;
  }
  if (row_ > 0) {
    --row_;
    col_ = static_cast<int>(lines_[static_cast<std::size_t>(row_)].size());
  }
}

void Document::move_right() {
  auto& line = lines_[static_cast<std::size_t>(row_)];
  if (col_ < static_cast<int>(line.size())) {
    col_ = static_cast<int>(utf8_next(line, static_cast<std::size_t>(col_)));
    return;
  }
  if (row_ + 1 < static_cast<int>(lines_.size())) {
    ++row_;
    col_ = 0;
  }
}

void Document::move_up() {
  if (row_ == 0) {
    return;
  }
  --row_;
  clamp();
}

void Document::move_down() {
  if (row_ + 1 >= static_cast<int>(lines_.size())) {
    return;
  }
  ++row_;
  clamp();
}

void Document::move_home() { col_ = 0; }

void Document::move_end() {
  col_ = static_cast<int>(lines_[static_cast<std::size_t>(row_)].size());
}

void Document::click_column(int line, int byte_pos) {
  row_ = line;
  if (row_ < 0) {
    row_ = 0;
  }
  if (row_ >= static_cast<int>(lines_.size())) {
    row_ = static_cast<int>(lines_.size()) - 1;
  }
  col_ = byte_pos;
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
  auto& line = lines_[static_cast<std::size_t>(row_)];
  if (col_ > static_cast<int>(line.size())) {
    col_ = static_cast<int>(line.size());
  }
  if (col_ < 0) {
    col_ = 0;
  }
}

}  // namespace sdl
