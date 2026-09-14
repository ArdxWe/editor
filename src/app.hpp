#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "context.hpp"
#include "document.hpp"
#include "explorer.hpp"
#include "font.hpp"
#include "renderer.hpp"
#include "syntax.hpp"
#include "terminal.hpp"
#include "texture.hpp"
#include "window.hpp"

namespace sdl {

// Top-level editor: file explorer, text view, PTY terminal, status bar, and
// draggable splitters.
class App {
 public:
  // `path` may be a file or a directory. When `prompt_folder` is true, a native
  // folder picker is shown after the window opens.
  explicit App(const std::filesystem::path& path, bool prompt_folder = false);
  ~App();
  void run();

 private:
  // Rasterized text run. Entries whose stamp does not match `cache_stamp_` are
  // evicted on the following frame.
  struct CachedText {
    std::string text;
    Uint32 fg = 0;
    Uint32 bg = 0;
    Uint32 stamp = 0;
    Texture texture;
  };

  // Input and documents
  void handle_event(const SDL_Event& event);
  void save();
  bool open_file(const std::filesystem::path& path);
  bool open_folder(const std::filesystem::path& path);
  void show_open_folder_dialog();
  static void folder_dialog_cb(void* userdata, const char* const* filelist,
                               int filter);
  int ask_unsaved();  // Save / Don't Save / Cancel

  // Rendering
  void draw();
  void draw_tree(int line_h, int content_bottom);
  void draw_editor(int line_h, int content_bottom);
  void draw_terminal(int line_h, int term_top, int status_y);
  void toggle_terminal();
  void handle_terminal_key(const SDL_Event& event);

  // Layout in renderer pixels. The status bar is anchored to the bottom edge.
  int status_bar_top() const;
  int terminal_top() const;
  int terminal_panel_height() const;
  int content_bottom() const;  // Bottom of explorer/editor; above the terminal
  void refresh_title();
  void wake_caret();
  bool caret_visible() const;
  void ensure_cursor_visible();
  void note_edit();
  int visible_rows() const;
  int sidebar_width() const;
  int gutter_width() const;
  int editor_left() const;
  int split_hit_px() const;

  // Font size: status-bar stepper and Cmd/Ctrl + '=', '-', '0'.
  float dpi_scale() const;
  void set_font_size(int pt);
  void reload_font();
  // -1 shrink, +1 grow, 2 reset to default, 0 miss.
  int font_ui_hit(float x, float y) const;

  // Splitters: vertical for the sidebar, horizontal for the terminal.
  enum class Split { None, Sidebar, Terminal };
  Split hit_split(float x, float y) const;
  void set_split_cursor(Split split);
  void apply_split_drag(float x, float y);
  int byte_at_x(const std::string& line, float x) const;
  const Texture* cached_texture(const char* text, SDL_Color fg, SDL_Color bg);

  Context context_;
  TtfContext ttf_;
  Window window_;
  Renderer renderer_;
  Font font_;
  int font_pt_ = 16;  // Logical point size; raster size is pt * dpi_scale().
  // Left edge of the stepper. Large until the first draw to avoid stray hits.
  float font_ui_x_ = 1e9f;
  float font_minus_x_ = 0;
  float font_num_x_ = 0;
  float font_plus_x_ = 0;
  Document doc_;
  Explorer explorer_;
  Highlighter highlighter_;
  Terminal terminal_;

  int scroll_ = 0;       // First visible editor row
  int tree_scroll_ = 0;  // First visible explorer row
  int term_scroll_ = 0;  // First visible terminal row
  int sidebar_w_ = 0;
  int term_h_ = 0;
  Split drag_ = Split::None;
  Split hover_ = Split::None;
  bool drag_moved_ = false;  // Distinguishes a status-bar click from a drag
  SDL_Cursor* cursor_ew_ = nullptr;
  SDL_Cursor* cursor_ns_ = nullptr;

  bool term_open_ = false;
  bool term_focus_ = false;
  bool needs_redraw_ = true;
  bool prompt_folder_ = false;   // Show the folder picker on the first frame
  bool picking_folder_ = false;  // Native dialog is still outstanding
  std::string folder_dialog_start_;
  Uint64 caret_tick_ = 0;
  Uint32 cache_stamp_ = 0;  // Bumped each frame; used to age the glyph cache
  std::string last_title_;
  std::vector<std::unique_ptr<CachedText>> text_cache_;
};

}  // namespace sdl
