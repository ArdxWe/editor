#include "app.hpp"

#include <algorithm>
#include <string>

#include "log.hpp"
#include "surface.hpp"
#include "utf8.hpp"

namespace sdl {

namespace {

constexpr SDL_Color kChrome{245, 244, 241, 255};
constexpr SDL_Color kPage{252, 252, 250, 255};
constexpr SDL_Color kGutterBg{252, 252, 250, 255};
constexpr SDL_Color kHair{226, 224, 218, 255};
constexpr SDL_Color kLineHi{244, 242, 236, 255};
constexpr SDL_Color kLineNo{168, 164, 156, 255};
constexpr SDL_Color kStatus{110, 106, 98, 255};
constexpr SDL_Color kTree{48, 46, 42, 255};
constexpr SDL_Color kTreeFile{108, 104, 96, 255};
constexpr SDL_Color kTreeBg{242, 241, 237, 255};
constexpr SDL_Color kTreeActive{226, 221, 210, 255};
constexpr SDL_Color kStatusBg{245, 244, 241, 255};
constexpr SDL_Color kTermBg{245, 244, 241, 255};
constexpr SDL_Color kTermFg{45, 43, 38, 255};
constexpr SDL_Color kAccent{47, 108, 196, 255};
constexpr int kPad = 12;
constexpr int kTreePadX = 10;
constexpr int kTreeIndent = 14;
constexpr int kFontPtMin = 10;
constexpr int kFontPtMax = 40;
constexpr int kFontPtDefault = 16;
constexpr std::size_t kTextCacheLimit = 2048;

void stroke_h(const Renderer& renderer, float x, float y, float w) {
  renderer.set_draw_color(kHair.r, kHair.g, kHair.b, 255);
  renderer.fill_rect(SDL_FRect{x, y, w, 1.f});
}

void stroke_v(const Renderer& renderer, float x, float y, float h) {
  renderer.set_draw_color(kHair.r, kHair.g, kHair.b, 255);
  renderer.fill_rect(SDL_FRect{x, y, 1.f, h});
}

Uint32 pack_color(SDL_Color color) {
  return (static_cast<Uint32>(color.r) << 24) |
         (static_cast<Uint32>(color.g) << 16) |
         (static_cast<Uint32>(color.b) << 8) | static_cast<Uint32>(color.a);
}

bool event_needs_redraw(Uint32 type) {
  switch (type) {
    case SDL_EVENT_MOUSE_MOTION:
    case SDL_EVENT_MOUSE_BUTTON_UP:
    case SDL_EVENT_KEY_UP:
    case SDL_EVENT_TEXT_EDITING:
      return false;
    default:
      return true;
  }
}

bool is_terminal_toggle(const SDL_Event& event) {
  if (event.type != SDL_EVENT_KEY_DOWN || event.key.repeat) {
    return false;
  }
  const SDL_Keymod mod = event.key.mod;
  const bool ctrl = (mod & SDL_KMOD_CTRL) != 0;
  const bool gui = (mod & SDL_KMOD_GUI) != 0;
  const bool shortcut = ctrl || gui;
  if (!shortcut) {
    return event.key.key == SDLK_F12;
  }
  if (event.key.key == SDLK_J || event.key.scancode == SDL_SCANCODE_J) {
    return true;
  }
  if (event.key.key == SDLK_GRAVE || event.key.scancode == SDL_SCANCODE_GRAVE) {
    return true;
  }
  return false;
}

bool is_app_shortcut(SDL_Keymod mod) { return (mod & SDL_KMOD_GUI) != 0; }

int tree_rows_top(int line_h) { return line_h + 8; }

int tree_index_at(float y, int line_h, int scroll) {
  const int top = tree_rows_top(line_h);
  if (y < static_cast<float>(top)) {
    return -1;
  }
  return scroll + static_cast<int>((y - static_cast<float>(top)) / line_h);
}

bool file_inside_root(const std::filesystem::path& file,
                      const std::filesystem::path& root) {
  std::error_code ec;
  const auto file_abs = std::filesystem::weakly_canonical(file, ec);
  if (ec) {
    return false;
  }
  const auto root_abs = std::filesystem::weakly_canonical(root, ec);
  if (ec) {
    return false;
  }
  auto file_it = file_abs.begin();
  for (const auto& part : root_abs) {
    if (file_it == file_abs.end() || *file_it != part) {
      return false;
    }
    ++file_it;
  }
  return true;
}

}  // namespace

App::App(const std::filesystem::path& path, bool prompt_folder)
    : context_(SDL_INIT_VIDEO) {
  std::error_code ec;
  auto abs = std::filesystem::absolute(path, ec);
  if (ec) {
    abs = path;
  }
  abs = abs.lexically_normal();
  const bool is_dir = std::filesystem::is_directory(abs, ec);

  window_ = Window{"editor", 1200, 720,
                   SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY};
  renderer_ = Renderer{window_};
  renderer_.set_vsync(1);

  font_pt_ = kFontPtDefault;  // Cmd+= / - / 0 and the status-bar stepper change this
  config_ = load_config();
  config_.font = resolve_font(config_.font);
  font_ =
      Font{config_.font.c_str(), static_cast<float>(font_pt_) * dpi_scale()};

  if (is_dir) {
    explorer_.set_root(abs);
  } else {
    auto root = abs.parent_path();
    if (root.empty()) {
      root = std::filesystem::current_path();
    }
    explorer_.set_root(root);
    doc_.load(abs);
    explorer_.reveal(abs);
    highlighter_.set_language(language_from_path(abs.string()));
  }

  window_.start_text_input();
  sidebar_w_ = kPad * 3 + font_.measure("xxxxxxxxxxxxxxxxxxxxxx");
  term_h_ = font_.line_skip() * 10 + kPad;
  cursor_ew_ = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_EW_RESIZE);
  cursor_ns_ = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NS_RESIZE);
  prompt_folder_ = prompt_folder;
  LOG_INFO("open {} as {}", abs.string(), is_dir ? "folder" : "file");
  LOG_DEBUG("dpi={:.2f} font_pt={} line_skip={}", dpi_scale(), font_pt_,
            font_.line_skip());
  refresh_title();
  wake_caret();
}

App::~App() {
  if (cursor_ew_) {
    SDL_DestroyCursor(cursor_ew_);
  }
  if (cursor_ns_) {
    SDL_DestroyCursor(cursor_ns_);
  }
}

void App::run() {
  if (prompt_folder_) {
    prompt_folder_ = false;
    show_open_folder_dialog();
  }
  bool running = true;
  int last_blink = -1;
  while (running) {
    SDL_Event event{};
    if (term_open_ && terminal_.poll()) {
      needs_redraw_ = true;
    }
    const int wait_ms = (term_open_ || drag_ != Split::None) ? 16 : 50;
    bool got_event = needs_redraw_ ? SDL_PollEvent(&event)
                                   : SDL_WaitEventTimeout(&event, wait_ms);
    while (got_event) {
      if (event.type == SDL_EVENT_QUIT) {
        LOG_INFO("quit");
        running = false;
        break;
      }
      SDL_ConvertEventToRenderCoordinates(renderer_.get(), &event);
      handle_event(event);
      if (event_needs_redraw(event.type)) {
        needs_redraw_ = true;
      }
      got_event = SDL_PollEvent(&event);
    }
    if (!running) {
      break;
    }

    const int blink = caret_visible() ? 1 : 0;
    if (blink != last_blink) {
      last_blink = blink;
      needs_redraw_ = true;
    }
    if (term_open_ && terminal_.poll()) {
      needs_redraw_ = true;
    }
    if (needs_redraw_) {
      draw();
      needs_redraw_ = false;
    }
  }
}

void App::refresh_title() {
  std::string title = explorer_.root().filename().string();
  if (title.empty()) {
    title = explorer_.root().string();
  }
  if (doc_.has_file()) {
    title += " — ";
    title += doc_.title();
  }
  if (title == last_title_) {
    return;
  }
  last_title_ = title;
  window_.set_title(title.c_str());
}

const Texture* App::cached_texture(const char* text, SDL_Color fg,
                                   SDL_Color bg) {
  if (!text || !*text) {
    return nullptr;
  }
  const Uint32 packed_fg = pack_color(fg);
  const Uint32 packed_bg = pack_color(bg);
  for (auto& entry : text_cache_) {
    if (entry->fg == packed_fg && entry->bg == packed_bg &&
        entry->text == text) {
      entry->stamp = cache_stamp_;
      return &entry->texture;
    }
  }
  SDL_Surface* raw = font_.render(text, fg, bg);
  if (!raw) {
    return nullptr;
  }
  Surface surface{raw};
  if (text_cache_.size() >= kTextCacheLimit) {
    for (auto it = text_cache_.begin(); it != text_cache_.end(); ++it) {
      if ((*it)->stamp != cache_stamp_) {
        text_cache_.erase(it);
        break;
      }
    }
  }
  text_cache_.push_back(std::make_unique<CachedText>(CachedText{
      text, packed_fg, packed_bg, cache_stamp_, Texture{renderer_, surface}}));
  return &text_cache_.back()->texture;
}

void App::note_edit() {
  highlighter_.invalidate(doc_.row());
  refresh_title();
  ensure_cursor_visible();
  wake_caret();
}

int App::ask_unsaved() {
  const SDL_MessageBoxButtonData buttons[] = {
      {SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 0, "Cancel"},
      {0, 1, "Don't Save"},
      {SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 2, "Save"},
  };
  const SDL_MessageBoxData data = {
      SDL_MESSAGEBOX_WARNING,
      window_.get(),
      "Unsaved changes",
      "Save changes before continuing?",
      3,
      buttons,
      nullptr,
  };
  int id = 0;
  if (!SDL_ShowMessageBox(&data, &id)) {
    LOG_WARN("unsaved prompt failed: {}", SDL_GetError());
    return 0;
  }
  LOG_DEBUG("unsaved prompt choice={}", id);
  return id;
}

bool App::open_file(const std::filesystem::path& path) {
  std::error_code ec;
  if (std::filesystem::is_directory(path, ec)) {
    return false;
  }
  if (doc_.has_file() && std::filesystem::equivalent(doc_.path(), path, ec) &&
      !ec) {
    return true;
  }
  if (doc_.dirty()) {
    const int choice = ask_unsaved();
    if (choice == 0) {
      return false;
    }
    if (choice == 2) {
      save();
    }
  }
  doc_.load(path);
  scroll_ = 0;
  highlighter_.set_language(language_from_path(path.string()));
  explorer_.reveal(path);
  refresh_title();
  wake_caret();
  LOG_INFO("file {}", path.string());
  return true;
}

bool App::open_folder(const std::filesystem::path& path) {
  std::error_code ec;
  auto abs = std::filesystem::absolute(path, ec);
  if (ec) {
    abs = path;
  }
  abs = abs.lexically_normal();
  if (!std::filesystem::is_directory(abs, ec)) {
    return false;
  }
  if (doc_.dirty()) {
    const int choice = ask_unsaved();
    if (choice == 0) {
      return false;
    }
    if (choice == 2) {
      save();
    }
  }

  const bool keep_file = doc_.has_file() && file_inside_root(doc_.path(), abs);
  explorer_.set_root(abs);
  tree_scroll_ = 0;
  if (keep_file) {
    explorer_.reveal(doc_.path());
  } else {
    doc_.close();
    scroll_ = 0;
    highlighter_.set_language(Language::None);
  }
  if (term_open_) {
    terminal_.stop();
    terminal_.start(explorer_.root());
    term_scroll_ = 0;
  }
  refresh_title();
  needs_redraw_ = true;
  LOG_INFO("folder {}", abs.string());
  return true;
}

void App::show_open_folder_dialog() {
  if (picking_folder_) {
    return;
  }
  picking_folder_ = true;
  folder_dialog_start_ = explorer_.root().string();
  LOG_DEBUG("folder dialog from {}", folder_dialog_start_);
  SDL_ShowOpenFolderDialog(
      &App::folder_dialog_cb, this, window_.get(),
      folder_dialog_start_.empty() ? nullptr : folder_dialog_start_.c_str(),
      false);
}

void App::folder_dialog_cb(void* userdata, const char* const* filelist, int) {
  auto* app = static_cast<App*>(userdata);
  if (!app) {
    return;
  }
  app->picking_folder_ = false;
  if (!filelist || !filelist[0]) {
    LOG_DEBUG("folder dialog cancelled");
    return;
  }
  LOG_DEBUG("folder dialog picked {}", filelist[0]);
  app->open_folder(filelist[0]);
}

void App::handle_event(const SDL_Event& event) {
  const int line_h = std::max(1, font_.line_skip());
  const int status_y = status_bar_top();
  const int term_y = terminal_top();
  const int bottom = content_bottom();
  const int side = sidebar_width();

  if (is_terminal_toggle(event)) {
    toggle_terminal();
    return;
  }

  if (event.type == SDL_EVENT_TEXT_INPUT) {
    if ((SDL_GetModState() & (SDL_KMOD_CTRL | SDL_KMOD_GUI | SDL_KMOD_ALT)) !=
        0) {
      return;
    }
    if (term_focus_) {
      terminal_.write(event.text.text);
      return;
    }
    if (!doc_.can_edit()) {
      return;
    }
    if ((SDL_GetModState() & (SDL_KMOD_CTRL | SDL_KMOD_GUI | SDL_KMOD_ALT)) ==
        0) {
      doc_.insert(event.text.text);
      note_edit();
    }
    return;
  }

  if (event.type == SDL_EVENT_MOUSE_MOTION) {
    const float x = event.motion.x;
    const float y = event.motion.y;
    if (drag_ != Split::None) {
      apply_split_drag(x, y);
      drag_moved_ = true;
      needs_redraw_ = true;
    } else {
      const Split next = hit_split(x, y);
      if (next != hover_) {
        hover_ = next;
        set_split_cursor(next);
        needs_redraw_ = true;
      }
    }
    return;
  }

  if (event.type == SDL_EVENT_MOUSE_BUTTON_UP &&
      event.button.button == SDL_BUTTON_LEFT) {
    if (drag_ == Split::Terminal && !drag_moved_ &&
        event.button.y >= static_cast<float>(status_y - 4)) {
      toggle_terminal();
    }
    drag_ = Split::None;
    drag_moved_ = false;
    SDL_CaptureMouse(false);
    hover_ = hit_split(event.button.x, event.button.y);
    set_split_cursor(hover_);
    needs_redraw_ = true;
    return;
  }

  if (event.type == SDL_EVENT_MOUSE_WHEEL) {
    if (font_ui_hit(event.wheel.mouse_x, event.wheel.mouse_y) != 0) {
      set_font_size(font_pt_ + (event.wheel.y > 0 ? 1 : -1));
      return;
    }
    if (term_open_ && event.wheel.mouse_y >= static_cast<float>(term_y) &&
        event.wheel.mouse_y < static_cast<float>(status_y)) {
      const int vis = std::max(1, (status_y - term_y - kPad) / line_h);
      const int max_scroll =
          std::max(0, static_cast<int>(terminal_.lines().size()) - vis);
      term_scroll_ -= static_cast<int>(event.wheel.y);
      term_scroll_ = std::clamp(term_scroll_, 0, max_scroll);
      return;
    }
    if (event.wheel.mouse_x < static_cast<float>(side) &&
        event.wheel.mouse_y < static_cast<float>(bottom)) {
      const int vis = std::max(1, (bottom - tree_rows_top(line_h)) / line_h);
      const int max_scroll =
          std::max(0, static_cast<int>(explorer_.rows().size()) - vis);
      tree_scroll_ -= static_cast<int>(event.wheel.y);
      tree_scroll_ = std::clamp(tree_scroll_, 0, max_scroll);
    } else if (event.wheel.mouse_y < static_cast<float>(bottom)) {
      scroll_ -= static_cast<int>(event.wheel.y);
      const int max_scroll =
          std::max(0, static_cast<int>(doc_.lines().size()) - 1);
      scroll_ = std::clamp(scroll_, 0, max_scroll);
    }
    return;
  }

  if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN &&
      event.button.button == SDL_BUTTON_LEFT) {
    const int font_hit = font_ui_hit(event.button.x, event.button.y);
    if (font_hit != 0) {
      if (font_hit == 2) {
        set_font_size(kFontPtDefault);
      } else {
        set_font_size(font_pt_ + font_hit);
      }
      return;
    }
    const Split split = hit_split(event.button.x, event.button.y);
    if (split != Split::None) {
      drag_ = split;
      drag_moved_ = false;
      set_split_cursor(split);
      SDL_CaptureMouse(true);
      return;
    }
    if (event.button.y >= static_cast<float>(status_y - 4)) {
      toggle_terminal();
      return;
    }
    if (term_open_ && event.button.y >= static_cast<float>(term_y)) {
      term_focus_ = true;
      if (!terminal_.running()) {
        terminal_.start(explorer_.root());
      }
      return;
    }
    term_focus_ = false;
    if (event.button.x < static_cast<float>(side)) {
      const int row = tree_index_at(event.button.y, line_h, tree_scroll_);
      if (row < 0) {
        return;
      }
      if (row >= static_cast<int>(explorer_.rows().size())) {
        show_open_folder_dialog();
        return;
      }
      const TreeRow& item = explorer_.rows()[static_cast<std::size_t>(row)];
      std::error_code dir_ec;
      const bool is_dir =
          item.is_dir || std::filesystem::is_directory(item.path, dir_ec);
      if (is_dir) {
        explorer_.toggle(row);
      } else {
        open_file(item.path);
      }
      return;
    }
    if (!doc_.has_file()) {
      return;
    }
    const int row =
        scroll_ + static_cast<int>((event.button.y - kPad) / line_h);
    const auto& lines = doc_.lines();
    int target = std::clamp(row, 0, static_cast<int>(lines.size()) - 1);
    const int gutter = gutter_width();
    const int col =
        byte_at_x(lines[static_cast<std::size_t>(target)],
                  event.button.x - static_cast<float>(editor_left() + gutter));
    doc_.click_column(target, col);
    ensure_cursor_visible();
    wake_caret();
    return;
  }

  if (event.type != SDL_EVENT_KEY_DOWN) {
    return;
  }

  if (event.key.key == SDLK_S && (event.key.mod & SDL_KMOD_GUI)) {
    save();
    return;
  }

  if (event.key.key == SDLK_O && (event.key.mod & SDL_KMOD_GUI)) {
    show_open_folder_dialog();
    return;
  }

  if (is_app_shortcut(event.key.mod)) {
    const SDL_Keycode key = event.key.key;
    if (key == SDLK_EQUALS || key == SDLK_PLUS || key == SDLK_KP_PLUS) {
      set_font_size(font_pt_ + 1);
      return;
    }
    if (key == SDLK_MINUS || key == SDLK_KP_MINUS) {
      set_font_size(font_pt_ - 1);
      return;
    }
    if (key == SDLK_0 || key == SDLK_KP_0) {
      set_font_size(kFontPtDefault);
      return;
    }
  }

  if (term_focus_) {
    handle_terminal_key(event);
    return;
  }

  if (!doc_.can_edit()) {
    return;
  }

  switch (event.key.key) {
    case SDLK_BACKSPACE:
      doc_.backspace();
      break;
    case SDLK_DELETE:
      doc_.erase_forward();
      break;
    case SDLK_RETURN:
      doc_.newline();
      break;
    case SDLK_TAB:
      doc_.insert("    ");
      break;
    case SDLK_LEFT:
      doc_.move_left();
      break;
    case SDLK_RIGHT:
      doc_.move_right();
      break;
    case SDLK_UP:
      doc_.move_up();
      break;
    case SDLK_DOWN:
      doc_.move_down();
      break;
    case SDLK_HOME:
      doc_.move_home();
      break;
    case SDLK_END:
      doc_.move_end();
      break;
    case SDLK_PAGEDOWN:
      for (int i = 0; i < visible_rows(); ++i) {
        doc_.move_down();
      }
      break;
    case SDLK_PAGEUP:
      for (int i = 0; i < visible_rows(); ++i) {
        doc_.move_up();
      }
      break;
    default:
      return;
  }
  note_edit();
}

void App::save() {
  LOG_INFO("save {}", doc_.path().string());
  doc_.save();
  refresh_title();
}

void App::toggle_terminal() {
  term_open_ = !term_open_;
  term_focus_ = term_open_;
  LOG_DEBUG("terminal {} focus={}", term_open_ ? "open" : "closed",
            term_focus_);
  if (term_open_ && !terminal_.running()) {
    terminal_.start(explorer_.root());
  }
  term_scroll_ = 0;
}

void App::handle_terminal_key(const SDL_Event& event) {
  if (event.key.key == SDLK_ESCAPE) {
    term_focus_ = false;
    return;
  }
  const bool ctrl =
      (event.key.mod & SDL_KMOD_CTRL) && !(event.key.mod & SDL_KMOD_GUI);
  if (ctrl && event.key.key == SDLK_C) {
    terminal_.write("\x03", 1);
    return;
  }
  if (ctrl && event.key.key == SDLK_D) {
    terminal_.write("\x04", 1);
    return;
  }
  if (ctrl && event.key.key == SDLK_L) {
    terminal_.write("\x0c", 1);
    return;
  }
  switch (event.key.key) {
    case SDLK_RETURN:
    case SDLK_KP_ENTER:
      terminal_.write("\r", 1);
      term_scroll_ = 0;
      break;
    case SDLK_BACKSPACE:
      terminal_.write("\x7f", 1);
      break;
    case SDLK_TAB:
      terminal_.write("\t", 1);
      break;
    case SDLK_LEFT:
      terminal_.write("\x1b[D", 3);
      break;
    case SDLK_RIGHT:
      terminal_.write("\x1b[C", 3);
      break;
    case SDLK_UP:
      terminal_.write("\x1b[A", 3);
      break;
    case SDLK_DOWN:
      terminal_.write("\x1b[B", 3);
      break;
    case SDLK_HOME:
      terminal_.write("\x1b[H", 3);
      break;
    case SDLK_END:
      terminal_.write("\x1b[F", 3);
      break;
    case SDLK_DELETE:
      terminal_.write("\x1b[3~", 4);
      break;
    default:
      break;
  }
}

int App::status_bar_top() const {
  const auto out = renderer_.output_size();
  const int line_h = std::max(1, font_.line_skip());
  return out.h - line_h - kPad;
}

int App::terminal_panel_height() const {
  if (!term_open_) {
    return 0;
  }
  const int line_h = std::max(1, font_.line_skip());
  const int min_h = line_h * 4;
  const int max_h = std::max(min_h, status_bar_top() - line_h * 6);
  const int want = term_h_ > 0 ? term_h_ : line_h * 10 + kPad;
  return std::clamp(want, min_h, max_h);
}

int App::terminal_top() const {
  return status_bar_top() - terminal_panel_height();
}

int App::content_bottom() const {
  return term_open_ ? terminal_top() : status_bar_top();
}

// Restart the 500ms blink so the caret is visible after typing or moving.
void App::wake_caret() { caret_tick_ = SDL_GetTicks(); }

bool App::caret_visible() const {
  return ((SDL_GetTicks() - caret_tick_) / 500) % 2 == 0;
}

int App::visible_rows() const {
  const int line_h = std::max(1, font_.line_skip());
  const int text_h = std::max(1, content_bottom() - kPad * 2);
  return std::max(1, text_h / line_h);
}

int App::sidebar_width() const {
  const int total = renderer_.output_size().w;
  const int min_w = kPad * 10;
  const int max_w = std::max(min_w, total - min_w * 2);
  const int want = sidebar_w_ > 0 ? sidebar_w_ : min_w;
  return std::clamp(want, min_w, max_w);
}

int App::split_hit_px() const { return std::max(10, font_.line_skip() / 3); }

float App::dpi_scale() const {
  const auto win = window_.size();
  const auto out = renderer_.output_size();
  return win.h > 0 ? static_cast<float>(out.h) / static_cast<float>(win.h)
                   : 1.f;
}

void App::reload_font() {
  font_ =
      Font{config_.font.c_str(), static_cast<float>(font_pt_) * dpi_scale()};
  text_cache_.clear();
  ++cache_stamp_;
  needs_redraw_ = true;
  ensure_cursor_visible();
}

void App::set_font_size(int pt) {
  pt = std::clamp(pt, kFontPtMin, kFontPtMax);
  if (pt == font_pt_) {
    return;
  }
  font_pt_ = pt;
  LOG_INFO("font_pt={}", font_pt_);
  reload_font();
}

int App::font_ui_hit(float x, float y) const {
  const int status_y = status_bar_top();
  if (y < static_cast<float>(status_y - 4) || x < font_ui_x_) {
    return 0;
  }
  if (x < font_num_x_) {
    return -1;
  }
  if (x >= font_plus_x_) {
    return 1;
  }
  return 2;
}

App::Split App::hit_split(float x, float y) const {
  const int hit = split_hit_px();
  const int status_y = status_bar_top();
  const int side = sidebar_width();
  const int bottom = content_bottom();
  const int h_line = term_open_ ? terminal_top() : status_y;
  if (y >= static_cast<float>(h_line - hit) &&
      y <= static_cast<float>(h_line + hit) &&
      y < static_cast<float>(status_y + hit)) {
    return Split::Terminal;
  }
  if (y < static_cast<float>(bottom) && x >= static_cast<float>(side - hit) &&
      x <= static_cast<float>(side + hit)) {
    return Split::Sidebar;
  }
  return Split::None;
}

void App::set_split_cursor(Split split) {
  if (split == Split::Sidebar && cursor_ew_) {
    SDL_SetCursor(cursor_ew_);
  } else if (split == Split::Terminal && cursor_ns_) {
    SDL_SetCursor(cursor_ns_);
  } else if (SDL_Cursor* def = SDL_GetDefaultCursor()) {
    SDL_SetCursor(def);
  }
}

void App::apply_split_drag(float x, float y) {
  const int line_h = std::max(1, font_.line_skip());
  if (drag_ == Split::Sidebar) {
    sidebar_w_ = static_cast<int>(x);
    return;
  }
  if (drag_ != Split::Terminal) {
    return;
  }
  const int status_y = status_bar_top();
  const int min_h = line_h * 4;
  int next = status_y - static_cast<int>(y);
  if (next < min_h / 2) {
    if (term_open_) {
      term_open_ = false;
      term_focus_ = false;
    }
    return;
  }
  if (!term_open_) {
    term_open_ = true;
    term_focus_ = true;
    if (!terminal_.running()) {
      terminal_.start(explorer_.root());
    }
  }
  term_h_ = next;
}

int App::editor_left() const { return sidebar_width(); }

void App::ensure_cursor_visible() {
  const int vis = visible_rows();
  if (doc_.row() < scroll_) {
    scroll_ = doc_.row();
  } else if (doc_.row() >= scroll_ + vis) {
    scroll_ = doc_.row() - vis + 1;
  }
  if (scroll_ < 0) {
    scroll_ = 0;
  }
}

int App::gutter_width() const {
  int digits = 1;
  for (int n = static_cast<int>(doc_.lines().size()); n >= 10; n /= 10) {
    ++digits;
  }
  const std::string sample(static_cast<std::size_t>(digits), '0');
  return kPad * 2 + font_.measure(sample.c_str());
}

int App::byte_at_x(const std::string& line, float x) const {
  if (x <= 0 || line.empty()) {
    return 0;
  }
  int left_w = 0;
  const std::size_t left =
      font_.fit(line.c_str(), static_cast<int>(x), &left_w);
  if (left >= line.size()) {
    return static_cast<int>(line.size());
  }
  const std::size_t right = utf8_next(line, left);
  const int right_w = font_.measure(line.c_str(), right);
  if (x > static_cast<float>(left_w + right_w) * 0.5f) {
    return static_cast<int>(right);
  }
  return static_cast<int>(left);
}

void App::draw_tree(int line_h, int content_bottom) {
  const int side = sidebar_width();
  renderer_.set_draw_color(kTreeBg.r, kTreeBg.g, kTreeBg.b, 255);
  const SDL_FRect bar{0.f, 0.f, static_cast<float>(side),
                      static_cast<float>(content_bottom)};
  renderer_.fill_rect(bar);

  const SDL_Rect clip{0, 0, side, content_bottom};
  renderer_.set_clip(&clip);

  std::string root_name = explorer_.root().filename().string();
  if (root_name.empty()) {
    root_name = explorer_.root().string();
  }
  if (root_name.empty()) {
    root_name = "Explorer";
  }
  const int header_h = tree_rows_top(line_h);
  if (const Texture* title =
          cached_texture(root_name.c_str(), kStatus, kTreeBg)) {
    const SDL_FRect dest{static_cast<float>(kTreePadX), 6.f, title->width(),
                         title->height()};
    renderer_.copy(*title, nullptr, &dest);
  }
  stroke_h(renderer_, 8.f, static_cast<float>(header_h - 5),
           static_cast<float>(std::max(0, side - 16)));

  const int chevron_w = std::max(font_.measure("▸ "), font_.measure("▾ ")) + 4;
  const auto& rows = explorer_.rows();
  const int vis = std::max(1, (content_bottom - header_h) / line_h);
  for (int i = 0; i < vis; ++i) {
    const int index = tree_scroll_ + i;
    if (index >= static_cast<int>(rows.size())) {
      break;
    }
    const TreeRow& row = rows[static_cast<std::size_t>(index)];
    const float y = static_cast<float>(header_h + i * line_h);
    const float x = static_cast<float>(kTreePadX + row.depth * kTreeIndent);
    const SDL_Color bg = row.active ? kTreeActive : kTreeBg;
    if (row.active) {
      renderer_.set_draw_color(kTreeActive.r, kTreeActive.g, kTreeActive.b,
                               255);
      const SDL_FRect hi{6.f, y, static_cast<float>(std::max(0, side - 12)),
                         static_cast<float>(line_h - 1)};
      renderer_.fill_rect(hi);
      renderer_.set_draw_color(kAccent.r, kAccent.g, kAccent.b, 255);
      renderer_.fill_rect(
          SDL_FRect{6.f, y, 2.f, static_cast<float>(line_h - 1)});
    }
    if (row.is_dir) {
      const char* mark = row.expanded ? "▾" : "▸";
      if (const Texture* chev = cached_texture(mark, kLineNo, bg)) {
        const SDL_FRect dest{x, y + 1.f, chev->width(), chev->height()};
        renderer_.copy(*chev, nullptr, &dest);
      }
    }
    const float name_x = x + static_cast<float>(chevron_w);
    const int max_w = side - static_cast<int>(name_x) - kTreePadX;
    std::string name = row.name;
    if (max_w > 0) {
      const std::size_t keep = font_.fit(name.c_str(), max_w);
      if (keep < name.size()) {
        name.resize(keep);
      }
    }
    const SDL_Color fg = row.is_dir ? kTree : kTreeFile;
    if (const Texture* tex = cached_texture(name.c_str(), fg, bg)) {
      const SDL_FRect dest{name_x, y, tex->width(), tex->height()};
      renderer_.copy(*tex, nullptr, &dest);
    }
  }
  renderer_.set_clip(nullptr);
  stroke_v(renderer_, static_cast<float>(side - 1), 0.f,
           static_cast<float>(content_bottom));
}

void App::draw_editor(int line_h, int content_bottom) {
  const int left = editor_left();
  const auto out = renderer_.output_size();
  renderer_.set_draw_color(kPage.r, kPage.g, kPage.b, 255);
  const SDL_FRect page{static_cast<float>(left), 0.f,
                       static_cast<float>(out.w - left),
                       static_cast<float>(content_bottom)};
  renderer_.fill_rect(page);

  if (!doc_.has_file()) {
    const char* hint = "Cmd+O or click the empty sidebar to open a folder";
    if (const Texture* tex = cached_texture(hint, kStatus, kPage)) {
      const SDL_FRect dest{static_cast<float>(left + kPad * 2),
                           static_cast<float>(kPad), tex->width(),
                           tex->height()};
      renderer_.copy(*tex, nullptr, &dest);
    }
    return;
  }

  const int vis = visible_rows();
  const auto& lines = doc_.lines();
  const int gutter = gutter_width();

  renderer_.set_draw_color(kGutterBg.r, kGutterBg.g, kGutterBg.b, 255);
  const SDL_FRect gutter_rect{static_cast<float>(left), 0.f,
                              static_cast<float>(gutter),
                              static_cast<float>(content_bottom)};
  renderer_.fill_rect(gutter_rect);

  for (int i = 0; i < vis; ++i) {
    const int index = scroll_ + i;
    if (index >= static_cast<int>(lines.size())) {
      break;
    }
    const float y = static_cast<float>(kPad + i * line_h);
    const auto& line = lines[static_cast<std::size_t>(index)];
    if (index == doc_.row()) {
      renderer_.set_draw_color(kLineHi.r, kLineHi.g, kLineHi.b, 255);
      renderer_.fill_rect(SDL_FRect{static_cast<float>(left), y - 1.f,
                                    static_cast<float>(out.w - left),
                                    static_cast<float>(line_h)});
    }
    const std::string num = std::to_string(index + 1);
    const SDL_Color num_bg = index == doc_.row() ? kLineHi : kGutterBg;
    if (const Texture* num_tex = cached_texture(num.c_str(), kLineNo, num_bg)) {
      const float nx =
          static_cast<float>(left + gutter - kPad) - num_tex->width();
      const SDL_FRect dest{nx, y, num_tex->width(), num_tex->height()};
      renderer_.copy(*num_tex, nullptr, &dest);
    }

    float x = static_cast<float>(left + gutter);
    const auto tokens = highlighter_.tokens(index, line, lines);
    const SDL_Color text_bg = index == doc_.row() ? kLineHi : kPage;
    for (const Token& token : tokens) {
      if (token.end <= token.begin || token.begin >= line.size()) {
        continue;
      }
      const std::size_t n = std::min(token.end, line.size()) - token.begin;
      const std::string piece = line.substr(token.begin, n);
      if (const Texture* tex =
              cached_texture(piece.c_str(), token_color(token.kind), text_bg)) {
        const SDL_FRect dest{x, y, tex->width(), tex->height()};
        renderer_.copy(*tex, nullptr, &dest);
        x += tex->width();
      } else if (!piece.empty()) {
        x += static_cast<float>(font_.measure(piece.c_str()));
      }
    }

    if (index == doc_.row() && caret_visible()) {
      const std::size_t col = static_cast<std::size_t>(doc_.line_offset());
      const int prefix_w = col == 0 ? 0 : font_.measure(line.c_str(), col);
      const float cx = static_cast<float>(left + gutter + prefix_w);
      const SDL_FRect caret{cx, y, 2.f, static_cast<float>(line_h)};
      renderer_.set_draw_color(kAccent.r, kAccent.g, kAccent.b, 255);
      renderer_.fill_rect(caret);
    }
  }
}

void App::draw_terminal(int line_h, int term_top, int status_y) {
  const auto out = renderer_.output_size();
  const int tab_h = line_h + 8;
  renderer_.set_draw_color(kTermBg.r, kTermBg.g, kTermBg.b, 255);
  const SDL_FRect panel{0.f, static_cast<float>(term_top),
                        static_cast<float>(out.w),
                        static_cast<float>(status_y - term_top)};
  renderer_.fill_rect(panel);
  stroke_h(renderer_, 0.f, static_cast<float>(term_top),
           static_cast<float>(out.w));

  const float tab_y = static_cast<float>(term_top + 4);
  if (const Texture* tab = cached_texture(
          "TERMINAL", term_focus_ ? kAccent : kStatus, kTermBg)) {
    const SDL_FRect dest{static_cast<float>(kPad), tab_y, tab->width(),
                         tab->height()};
    renderer_.copy(*tab, nullptr, &dest);
    if (term_focus_) {
      renderer_.set_draw_color(kAccent.r, kAccent.g, kAccent.b, 255);
      renderer_.fill_rect(SDL_FRect{static_cast<float>(kPad),
                                    static_cast<float>(term_top + tab_h - 2),
                                    tab->width(), 2.f});
    }
  }
  stroke_h(renderer_, 0.f, static_cast<float>(term_top + tab_h),
           static_cast<float>(out.w));

  const int body_top = term_top + tab_h;
  const int m_w = std::max(1, font_.measure("M"));
  const int cols = std::max(20, (out.w - kPad * 2) / m_w);
  const int rows = std::max(4, (status_y - body_top - 4) / line_h);
  terminal_.resize(cols, rows);

  const SDL_Rect clip{0, body_top, out.w, status_y - body_top};
  renderer_.set_clip(&clip);

  const auto& lines = terminal_.lines();
  const int vis = std::max(1, rows);
  const int max_scroll = std::max(0, static_cast<int>(lines.size()) - vis);
  term_scroll_ = std::clamp(term_scroll_, 0, max_scroll);
  const int first =
      std::max(0, static_cast<int>(lines.size()) - vis - term_scroll_);

  for (int i = 0; i < vis; ++i) {
    const int index = first + i;
    if (index >= static_cast<int>(lines.size())) {
      break;
    }
    const float y = static_cast<float>(body_top + 4 + i * line_h);
    const auto& line = lines[static_cast<std::size_t>(index)];
    if (!line.empty()) {
      if (const Texture* tex = cached_texture(line.c_str(), kTermFg, kTermBg)) {
        const SDL_FRect dest{static_cast<float>(kPad), y, tex->width(),
                             tex->height()};
        renderer_.copy(*tex, nullptr, &dest);
      }
    }
    if (term_focus_ && index == terminal_.cursor_row() && caret_visible()) {
      const std::size_t col =
          static_cast<std::size_t>(std::max(0, terminal_.cursor_col()));
      const int prefix_w =
          col == 0 || line.empty()
              ? 0
              : font_.measure(line.c_str(), std::min(col, line.size()));
      const SDL_FRect caret{static_cast<float>(kPad + prefix_w), y, 2.f,
                            static_cast<float>(line_h)};
      renderer_.set_draw_color(kAccent.r, kAccent.g, kAccent.b, 255);
      renderer_.fill_rect(caret);
    }
  }
  renderer_.set_clip(nullptr);
}

void App::draw() {
  ++cache_stamp_;
  renderer_.set_draw_color(kChrome.r, kChrome.g, kChrome.b, 255);
  renderer_.clear();

  const auto out = renderer_.output_size();
  const int line_h = std::max(1, font_.line_skip());
  const int status_y = status_bar_top();
  const int bottom = content_bottom();

  draw_tree(line_h, bottom);
  draw_editor(line_h, bottom);
  if (term_open_) {
    draw_terminal(line_h, terminal_top(), status_y);
  }

  const int side = sidebar_width();
  const bool side_hot = hover_ == Split::Sidebar || drag_ == Split::Sidebar;
  const bool term_hot = hover_ == Split::Terminal || drag_ == Split::Terminal;
  if (side_hot) {
    renderer_.set_draw_color(kAccent.r, kAccent.g, kAccent.b, 255);
    renderer_.fill_rect(SDL_FRect{static_cast<float>(side - 2), 0.f, 4.f,
                                  static_cast<float>(bottom)});
  }
  if (term_hot || term_open_) {
    const float hy = static_cast<float>(term_open_ ? terminal_top() : status_y);
    if (term_hot) {
      renderer_.set_draw_color(kAccent.r, kAccent.g, kAccent.b, 255);
      renderer_.fill_rect(
          SDL_FRect{0.f, hy - 2.f, static_cast<float>(out.w), 4.f});
    } else {
      stroke_h(renderer_, 0.f, hy, static_cast<float>(out.w));
    }
  }

  renderer_.set_draw_color(kStatusBg.r, kStatusBg.g, kStatusBg.b, 255);
  const SDL_FRect bar{0.f, static_cast<float>(status_y - 4),
                      static_cast<float>(out.w),
                      static_cast<float>(line_h + kPad + 4)};
  renderer_.fill_rect(bar);
  stroke_h(renderer_, 0.f, static_cast<float>(status_y - 4),
           static_cast<float>(out.w));

  std::string hint;
  if (doc_.has_file()) {
    std::error_code ec;
    auto rel = std::filesystem::relative(doc_.path(), explorer_.root(), ec);
    hint = (!ec && !rel.empty()) ? rel.generic_string()
                                 : doc_.path().filename().string();
    hint += "    ";
  }
  hint += "Cmd+O folder    Cmd+S save    Cmd+J terminal";
  if (term_focus_) {
    hint += "    Esc editor";
  }
  if (const Texture* hint_tex =
          cached_texture(hint.c_str(), kStatus, kStatusBg)) {
    const SDL_FRect dest{static_cast<float>(kPad), static_cast<float>(status_y),
                         hint_tex->width(), hint_tex->height()};
    renderer_.copy(*hint_tex, nullptr, &dest);
  }

  const std::string minus = "-";
  const std::string plus = "+";
  const std::string size_label = std::to_string(font_pt_);
  const int minus_w = std::max(1, font_.measure(minus.c_str()));
  const int plus_w = std::max(1, font_.measure(plus.c_str()));
  const int num_w = std::max(1, font_.measure(size_label.c_str()));
  const int gap = 10;
  const float block_w = static_cast<float>(minus_w + num_w + plus_w + gap * 4);
  font_ui_x_ = std::max(static_cast<float>(kPad),
                        static_cast<float>(out.w) - kPad - block_w);
  font_minus_x_ = font_ui_x_;
  font_num_x_ = font_minus_x_ + static_cast<float>(minus_w + gap);
  font_plus_x_ = font_num_x_ + static_cast<float>(num_w + gap);
  const float ui_y = static_cast<float>(status_y);
  auto blit = [&](const char* text, float x) {
    if (const Texture* tex = cached_texture(text, kStatus, kStatusBg)) {
      const SDL_FRect dest{x, ui_y, tex->width(), tex->height()};
      renderer_.copy(*tex, nullptr, &dest);
    }
  };
  blit(minus.c_str(), font_minus_x_);
  blit(size_label.c_str(), font_num_x_);
  blit(plus.c_str(), font_plus_x_);

  renderer_.present();
}

}  // namespace sdl
