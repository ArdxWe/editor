#pragma once

#include "context.hpp"
#include "document.hpp"
#include "explorer.hpp"
#include "font.hpp"
#include "renderer.hpp"
#include "syntax.hpp"
#include "terminal.hpp"
#include "texture.hpp"
#include "window.hpp"

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace sdl {

class App {
public:
    explicit App(const std::filesystem::path& path);
    ~App();
    void run();

private:
    struct CachedText {
        std::string text;
        Uint32 fg = 0;
        Uint32 bg = 0;
        Uint32 stamp = 0;
        Texture texture;
    };

    void handle_event(const SDL_Event& event);
    void save();
    bool open_file(const std::filesystem::path& path);
    int ask_unsaved();
    void draw();
    void draw_tree(int line_h, int content_bottom);
    void draw_editor(int line_h, int content_bottom);
    void draw_terminal(int line_h, int term_top, int status_y);
    void toggle_terminal();
    void handle_terminal_key(const SDL_Event& event);
    int status_bar_top() const;
    int terminal_top() const;
    int terminal_panel_height() const;
    int content_bottom() const;
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
    Document doc_;
    Explorer explorer_;
    Highlighter highlighter_;
    Terminal terminal_;
    int scroll_ = 0;
    int tree_scroll_ = 0;
    int term_scroll_ = 0;
    int sidebar_w_ = 0;
    int term_h_ = 0;
    Split drag_ = Split::None;
    Split hover_ = Split::None;
    bool drag_moved_ = false;
    SDL_Cursor* cursor_ew_ = nullptr;
    SDL_Cursor* cursor_ns_ = nullptr;
    bool term_open_ = false;
    bool term_focus_ = false;
    bool needs_redraw_ = true;
    Uint64 caret_tick_ = 0;
    Uint32 cache_stamp_ = 0;
    std::string last_title_;
    std::vector<std::unique_ptr<CachedText>> text_cache_;
};

}  // namespace sdl
