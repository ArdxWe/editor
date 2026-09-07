#include "terminal.hpp"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <string_view>

#include "utf8.hpp"

#ifndef _WIN32
#include <fcntl.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>
#endif

namespace sdl {

namespace {

constexpr int kMaxScrollback = 2000;
constexpr int kGround = 0;
constexpr int kEsc = 1;
constexpr int kCsi = 2;
constexpr int kOsc = 3;

bool is_utf8_start(unsigned char c) {
  return c < 0x80 || (c >= 0xC2 && c < 0xF5);
}

std::size_t utf8_len(unsigned char c) {
  if (c < 0x80) {
    return 1;
  }
  if ((c & 0xE0) == 0xC0) {
    return 2;
  }
  if ((c & 0xF0) == 0xE0) {
    return 3;
  }
  if ((c & 0xF8) == 0xF0) {
    return 4;
  }
  return 1;
}

}  // namespace

Terminal::~Terminal() { stop(); }

#ifndef _WIN32

bool Terminal::start(const std::filesystem::path& cwd) {
  stop();
  master_ = posix_openpt(O_RDWR | O_NOCTTY);
  if (master_ < 0) {
    return false;
  }
  if (grantpt(master_) != 0 || unlockpt(master_) != 0) {
    close(master_);
    master_ = -1;
    return false;
  }
  const char* slave_name = ptsname(master_);
  if (!slave_name) {
    close(master_);
    master_ = -1;
    return false;
  }
  const int slave = open(slave_name, O_RDWR | O_NOCTTY);
  if (slave < 0) {
    close(master_);
    master_ = -1;
    return false;
  }

  const pid_t pid = fork();
  if (pid < 0) {
    close(slave);
    close(master_);
    master_ = -1;
    return false;
  }
  if (pid == 0) {
    close(master_);
    setsid();
    ioctl(slave, TIOCSCTTY, 0);
    dup2(slave, STDIN_FILENO);
    dup2(slave, STDOUT_FILENO);
    dup2(slave, STDERR_FILENO);
    if (slave > 2) {
      close(slave);
    }
    if (!cwd.empty()) {
      chdir(cwd.c_str());
    }
    setenv("TERM", "xterm-256color", 1);
    setenv("COLORTERM", "truecolor", 1);
    const char* shell = std::getenv("SHELL");
    if (!shell || !*shell) {
#ifdef __APPLE__
      shell = "/bin/zsh";
#else
      shell = "/bin/bash";
#endif
    }
    execl(shell, shell, "-i", static_cast<char*>(nullptr));
    _exit(127);
  }

  close(slave);
  pid_ = pid;
  const int flags = fcntl(master_, F_GETFL, 0);
  fcntl(master_, F_SETFL, flags | O_NONBLOCK);
  lines_.assign(1, "");
  row_ = 0;
  col_ = 0;
  parse_ = kGround;
  pending_.clear();
  resize(cols_, rows_);
  return true;
}

void Terminal::stop() {
  if (pid_ > 0) {
    kill(static_cast<pid_t>(pid_), SIGHUP);
    int status = 0;
    waitpid(static_cast<pid_t>(pid_), &status, 0);
    pid_ = -1;
  }
  if (master_ >= 0) {
    close(master_);
    master_ = -1;
  }
}

void Terminal::write(const char* data, std::size_t size) {
  if (master_ < 0 || !data || size == 0) {
    return;
  }
  std::size_t off = 0;
  while (off < size) {
    const ssize_t n = ::write(master_, data + off, size - off);
    if (n < 0) {
      if (errno == EAGAIN || errno == EINTR) {
        continue;
      }
      break;
    }
    off += static_cast<std::size_t>(n);
  }
}

bool Terminal::poll() {
  if (master_ < 0) {
    return false;
  }
  if (pid_ > 0) {
    int status = 0;
    if (waitpid(static_cast<pid_t>(pid_), &status, WNOHANG) == pid_) {
      pid_ = -1;
      close(master_);
      master_ = -1;
      pending_ += "\n[shell exited]\n";
      feed(pending_.data(), pending_.size());
      pending_.clear();
      return true;
    }
  }

  std::array<char, 4096> buf{};
  bool dirty = false;
  for (;;) {
    const ssize_t n = ::read(master_, buf.data(), buf.size());
    if (n > 0) {
      feed(buf.data(), static_cast<std::size_t>(n));
      dirty = true;
      continue;
    }
    if (n < 0 && (errno == EAGAIN || errno == EINTR)) {
      break;
    }
    if (n == 0) {
      stop();
      break;
    }
    break;
  }
  return dirty;
}

void Terminal::resize(int cols, int rows) {
  cols_ = std::max(20, cols);
  rows_ = std::max(4, rows);
  if (master_ < 0) {
    return;
  }
  winsize ws{};
  ws.ws_col = static_cast<unsigned short>(cols_);
  ws.ws_row = static_cast<unsigned short>(rows_);
  ioctl(master_, TIOCSWINSZ, &ws);
}

#else

bool Terminal::start(const std::filesystem::path&) { return false; }
void Terminal::stop() {}
void Terminal::write(const char*, std::size_t) {}
bool Terminal::poll() { return false; }
void Terminal::resize(int cols, int rows) {
  cols_ = cols;
  rows_ = rows;
}

#endif

void Terminal::ensure_line() {
  if (lines_.empty()) {
    lines_.emplace_back();
  }
  if (row_ >= static_cast<int>(lines_.size())) {
    row_ = static_cast<int>(lines_.size()) - 1;
  }
  if (row_ < 0) {
    row_ = 0;
  }
}

void Terminal::newline() {
  ++row_;
  if (row_ >= static_cast<int>(lines_.size())) {
    lines_.emplace_back();
  }
  col_ = 0;
  if (static_cast<int>(lines_.size()) > kMaxScrollback) {
    const int drop = static_cast<int>(lines_.size()) - kMaxScrollback;
    lines_.erase(lines_.begin(), lines_.begin() + drop);
    row_ = std::max(0, row_ - drop);
  }
}

void Terminal::erase_to_end() {
  ensure_line();
  auto& line = lines_[static_cast<std::size_t>(row_)];
  if (col_ < static_cast<int>(line.size())) {
    line.resize(static_cast<std::size_t>(std::max(0, col_)));
  }
}

void Terminal::put_utf8(std::string_view ch) {
  ensure_line();
  auto& line = lines_[static_cast<std::size_t>(row_)];
  if (col_ > static_cast<int>(line.size())) {
    line.append(static_cast<std::size_t>(col_) - line.size(), ' ');
  }
  if (col_ == static_cast<int>(line.size())) {
    line.append(ch);
    col_ += static_cast<int>(ch.size());
    return;
  }
  const std::size_t next = utf8_next(line, static_cast<std::size_t>(col_));
  line.replace(static_cast<std::size_t>(col_),
               next - static_cast<std::size_t>(col_), ch);
  col_ += static_cast<int>(ch.size());
}

void Terminal::feed(const char* data, std::size_t size) {
  pending_.append(data, size);
  std::size_t i = 0;
  while (i < pending_.size()) {
    const unsigned char c = static_cast<unsigned char>(pending_[i]);
    if (parse_ == kEsc) {
      if (c == '[') {
        parse_ = kCsi;
        ++i;
        continue;
      }
      if (c == ']') {
        parse_ = kOsc;
        ++i;
        continue;
      }
      parse_ = kGround;
      ++i;
      continue;
    }
    if (parse_ == kCsi) {
      if (c >= 0x40 && c <= 0x7E) {
        if (c == 'K') {
          erase_to_end();
        } else if (c == 'C') {
          ++col_;
        } else if (c == 'D' && col_ > 0) {
          col_ =
              static_cast<int>(utf8_prev(lines_[static_cast<std::size_t>(row_)],
                                         static_cast<std::size_t>(col_)));
        }
        parse_ = kGround;
      }
      ++i;
      continue;
    }
    if (parse_ == kOsc) {
      if (c == 0x07 || c == '\\') {
        parse_ = kGround;
      }
      ++i;
      continue;
    }

    if (c == 0x1B) {
      parse_ = kEsc;
      ++i;
      continue;
    }
    if (c == '\n') {
      newline();
      ++i;
      continue;
    }
    if (c == '\r') {
      col_ = 0;
      ++i;
      continue;
    }
    if (c == '\b') {
      ensure_line();
      if (col_ > 0) {
        col_ =
            static_cast<int>(utf8_prev(lines_[static_cast<std::size_t>(row_)],
                                       static_cast<std::size_t>(col_)));
      }
      ++i;
      continue;
    }
    if (c == '\a' || c == '\0') {
      ++i;
      continue;
    }
    if (c == '\t') {
      const int next = (col_ / 8 + 1) * 8;
      while (col_ < next) {
        put_utf8(" ");
      }
      ++i;
      continue;
    }
    if (c < 0x20) {
      ++i;
      continue;
    }
    if (!is_utf8_start(c)) {
      ++i;
      continue;
    }
    const std::size_t n = utf8_len(c);
    if (i + n > pending_.size()) {
      break;
    }
    put_utf8(std::string_view{pending_.data() + i, n});
    i += n;
  }
  pending_.erase(0, i);
}

}  // namespace sdl
