#include "terminal.hpp"

#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <string_view>
#include <vector>

namespace sdl {

namespace {

constexpr int kMaxScrollback = 2000;
// Tiny VT parser: Ground is printable text. ESC switches to Esc, then
// '[' is CSI (cursor/erase) and ']' is OSC (window title etc., ignored).
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

constexpr SDL_Color kAnsi[16] = {
    {0, 0, 0, 255},       {205, 49, 49, 255},   {13, 188, 121, 255},
    {229, 181, 51, 255},  {36, 114, 200, 255},  {188, 63, 188, 255},
    {17, 168, 205, 255},  {204, 204, 204, 255}, {102, 102, 102, 255},
    {241, 76, 76, 255},   {35, 209, 139, 255},  {245, 205, 77, 255},
    {59, 142, 234, 255},  {214, 112, 214, 255}, {41, 184, 219, 255},
    {229, 229, 229, 255},
};

SDL_Color color_256(int n) {
  n = std::clamp(n, 0, 255);
  if (n < 16) {
    return kAnsi[n];
  }
  if (n < 232) {
    n -= 16;
    const int r = n / 36;
    const int g = (n / 6) % 6;
    const int b = n % 6;
    auto level = [](int v) -> Uint8 {
      return v == 0 ? 0 : static_cast<Uint8>(55 + 40 * v);
    };
    return {level(r), level(g), level(b), 255};
  }
  const Uint8 v = static_cast<Uint8>(8 + (n - 232) * 10);
  return {v, v, v, 255};
}

SDL_Color color_rgb(int r, int g, int b) {
  return {static_cast<Uint8>(std::clamp(r, 0, 255)),
          static_cast<Uint8>(std::clamp(g, 0, 255)),
          static_cast<Uint8>(std::clamp(b, 0, 255)), 255};
}

std::vector<int> parse_csi_params(const std::string& s) {
  std::vector<int> out;
  int v = 0;
  bool any = false;
  for (char c : s) {
    if (c == ';' || c == ':') {
      out.push_back(any ? v : 0);
      v = 0;
      any = false;
      continue;
    }
    if (c >= '0' && c <= '9') {
      v = v * 10 + (c - '0');
      any = true;
    }
  }
  out.push_back(any ? v : 0);
  return out;
}

}  // namespace

Terminal::~Terminal() { stop(); }

bool Terminal::start(const std::filesystem::path& cwd) {
  stop();

  // A PTY is a kernel pipe that looks like a real tty to the child. We keep
  // the master fd and talk to the shell through it; the child gets the slave.
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
    // Child: drop the master, become a session leader, and make the slave
    // the controlling tty so job control / SIGINT work. Then stdin/out/err
    // all go through that fd and we exec an interactive shell.
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
    setenv("CLICOLOR", "1", 0);
    const char* shell = std::getenv("SHELL");
    if (!shell || !*shell) {
      shell = "/bin/zsh";
    }
    execl(shell, shell, "-i", static_cast<char*>(nullptr));
    _exit(127);
  }

  // Parent: only the master is needed. Non-blocking so poll() can drain
  // output without stalling the editor.
  close(slave);
  pid_ = pid;
  const int flags = fcntl(master_, F_GETFL, 0);
  fcntl(master_, F_SETFL, flags | O_NONBLOCK);
  lines_.assign(1, {});
  row_ = 0;
  col_ = 0;
  parse_ = kGround;
  pending_.clear();
  csi_.clear();
  reset_pen();
  resize(cols_, rows_);
  return true;
}

void Terminal::stop() {
  if (pid_ > 0) {
    const pid_t pid = static_cast<pid_t>(pid_);
    pid_ = -1;
    kill(pid, SIGHUP);
    int status = 0;
    bool reaped = false;
    // Give the shell a moment to exit, then SIGKILL so quit cannot hang.
    for (int i = 0; i < 50; ++i) {
      const pid_t got = waitpid(pid, &status, WNOHANG);
      if (got == pid || (got < 0 && errno != EINTR)) {
        reaped = true;
        break;
      }
      usleep(10 * 1000);
    }
    if (!reaped) {
      kill(pid, SIGKILL);
      while (waitpid(pid, &status, 0) < 0 && errno == EINTR) {
      }
    }
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
  int waits = 0;
  while (off < size) {
    const ssize_t n = ::write(master_, data + off, size - off);
    if (n > 0) {
      off += static_cast<std::size_t>(n);
      waits = 0;
      continue;
    }
    if (n < 0 && errno == EINTR) {
      continue;
    }
    // PTY buffer full: wait briefly, then drop the rest instead of spinning.
    if (n < 0 && errno == EAGAIN && waits < 4) {
      ++waits;
      pollfd pfd{};
      pfd.fd = master_;
      pfd.events = POLLOUT;
      ::poll(&pfd, 1, 5);
      continue;
    }
    break;
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

void Terminal::erase_display() {
  lines_.assign(1, {});
  row_ = 0;
  col_ = 0;
}

void Terminal::reset_pen() {
  pen_fg_ = kTermFg;
  pen_bg_ = kTermBg;
  bold_ = false;
  inverse_ = false;
}

SDL_Color Terminal::paint_fg() const {
  SDL_Color c = inverse_ ? pen_bg_ : pen_fg_;
  if (bold_) {
    c.r = static_cast<Uint8>(c.r + (255 - c.r) / 3);
    c.g = static_cast<Uint8>(c.g + (255 - c.g) / 3);
    c.b = static_cast<Uint8>(c.b + (255 - c.b) / 3);
  }
  return c;
}

SDL_Color Terminal::paint_bg() const { return inverse_ ? pen_fg_ : pen_bg_; }

void Terminal::put_utf8(std::string_view ch) {
  ensure_line();
  auto& line = lines_[static_cast<std::size_t>(row_)];
  while (static_cast<int>(line.size()) < col_) {
    line.push_back(TermCell{" ", kTermFg, kTermBg});
  }
  TermCell cell{std::string{ch}, paint_fg(), paint_bg()};
  if (col_ == static_cast<int>(line.size())) {
    line.push_back(std::move(cell));
  } else {
    line[static_cast<std::size_t>(col_)] = std::move(cell);
  }
  ++col_;
}

void Terminal::apply_sgr(const std::vector<int>& params) {
  if (params.empty()) {
    reset_pen();
    return;
  }
  for (std::size_t i = 0; i < params.size(); ++i) {
    const int n = params[i];
    if (n == 0) {
      reset_pen();
    } else if (n == 1) {
      bold_ = true;
    } else if (n == 22) {
      bold_ = false;
    } else if (n == 7) {
      inverse_ = true;
    } else if (n == 27) {
      inverse_ = false;
    } else if (n == 39) {
      pen_fg_ = kTermFg;
    } else if (n == 49) {
      pen_bg_ = kTermBg;
    } else if (n >= 30 && n <= 37) {
      pen_fg_ = kAnsi[n - 30];
    } else if (n >= 90 && n <= 97) {
      pen_fg_ = kAnsi[n - 90 + 8];
    } else if (n >= 40 && n <= 47) {
      pen_bg_ = kAnsi[n - 40];
    } else if (n >= 100 && n <= 107) {
      pen_bg_ = kAnsi[n - 100 + 8];
    } else if (n == 38 || n == 48) {
      const bool is_fg = n == 38;
      if (i + 1 >= params.size()) {
        continue;
      }
      const int mode = params[++i];
      SDL_Color color = is_fg ? kTermFg : kTermBg;
      if (mode == 5 && i + 1 < params.size()) {
        color = color_256(params[++i]);
      } else if (mode == 2 && i + 3 < params.size()) {
        color = color_rgb(params[i + 1], params[i + 2], params[i + 3]);
        i += 3;
      }
      if (is_fg) {
        pen_fg_ = color;
      } else {
        pen_bg_ = color;
      }
    }
  }
}

void Terminal::apply_csi(char final) {
  if (!csi_.empty() && csi_[0] == '?') {
    csi_.clear();
    return;
  }
  const std::vector<int> p = parse_csi_params(csi_);
  csi_.clear();
  const int a = p.empty() ? 0 : p[0];
  if (final == 'm') {
    apply_sgr(p);
    return;
  }
  if (final == 'K') {
    if (a == 2) {
      ensure_line();
      lines_[static_cast<std::size_t>(row_)].clear();
    } else if (a == 1) {
      ensure_line();
      auto& line = lines_[static_cast<std::size_t>(row_)];
      const int n = std::min(col_, static_cast<int>(line.size()));
      for (int i = 0; i < n; ++i) {
        line[static_cast<std::size_t>(i)] = TermCell{" ", kTermFg, kTermBg};
      }
    } else {
      erase_to_end();
    }
    return;
  }
  if (final == 'J' && (a == 2 || a == 3)) {
    erase_display();
    return;
  }
  if (final == 'C') {
    col_ += std::max(1, a == 0 ? 1 : a);
    return;
  }
  if (final == 'D') {
    col_ = std::max(0, col_ - std::max(1, a == 0 ? 1 : a));
  }
}

void Terminal::feed(const char* data, std::size_t size) {
  // Reads can split a UTF-8 code point or an escape sequence, so leftovers
  // stay in pending_ until the next poll().
  pending_.append(data, size);
  std::size_t i = 0;
  while (i < pending_.size()) {
    const unsigned char c = static_cast<unsigned char>(pending_[i]);
    if (parse_ == kEsc) {
      if (c == '[') {
        parse_ = kCsi;
        csi_.clear();
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
      // CSI ends on a byte in '@'..'~'. SGR (m) carries zsh/ls colors.
      if (c >= 0x40 && c <= 0x7E) {
        apply_csi(static_cast<char>(c));
        parse_ = kGround;
      } else if (csi_.size() < 64) {
        csi_.push_back(static_cast<char>(c));
      }
      ++i;
      continue;
    }
    if (parse_ == kOsc) {
      // OSC is terminated by BEL or ST (ESC \). We skip the payload.
      if (c == 0x07 || c == '\\') {
        parse_ = kGround;
      }
      ++i;
      continue;
    }

    // Ground: ESC starts a sequence; otherwise C0 controls or UTF-8 text.
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
      if (col_ > 0) {
        --col_;
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
      break;  // Incomplete UTF-8; wait for more bytes.
    }
    put_utf8(std::string_view{pending_.data() + i, n});
    i += n;
  }
  pending_.erase(0, i);
}

}  // namespace sdl
