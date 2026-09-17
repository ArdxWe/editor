# editor

An SDL3 text editor for macOS: sidebar file tree, syntax highlighting, and a built-in PTY terminal.

## Dependencies

```bash
brew install cmake sdl3 sdl3_ttf spdlog
```

Requires C++20 and Apple Clang. The Makefile defaults to a local proxy at `http://127.0.0.1:7897`. Without a proxy:

```bash
make http_proxy= https_proxy= ALL_PROXY=
```

## Commands

```bash
make          # Configure and build to build/editor
make run      # Build, then launch; pass args with make run ARGS=src/app.cpp
make format   # clang-format src/
make clean    # Remove build/
```

With no arguments, the editor opens the current directory. You can also open a file or folder directly:

```bash
./build/editor src/app.cpp
./build/editor .
```

## Configuration

`editor.conf` is copied into `build/`. Paths are relative to the **build directory**:

```
font = ../assets/fonts/jetbrains-mono-regular.ttf
```

The repo also ships `jetbrains-mono-medium.ttf`. CJK glyphs fall back to system fonts (PingFang, etc.).

Default font size is 16pt (range 10–40). Adjust with the status-bar `- 16 +` controls, or `Cmd+=` / `Cmd+-` / `Cmd+0`.

Logs go under `logs/`.

## Shortcuts

| Key | Action |
|---|---|
| `Cmd+O` | Open folder |
| `Cmd+S` | Save |
| `Cmd+J` / `` Cmd+` `` / `F12` | Toggle terminal |
| `Cmd+=` / `Cmd+-` / `Cmd+0` | Zoom in / out / reset font size |
| `Esc` | Return focus from terminal to editor |

Sidebar and terminal panels have draggable splitters. Clicking an empty sidebar also opens a folder.
