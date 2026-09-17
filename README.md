# editor

macOS 上的 SDL3 文本编辑器：侧栏文件树、语法高亮、内置 PTY 终端。

## 依赖

```bash
brew install cmake sdl3 sdl3_ttf spdlog
```

需要 C++20 和 Apple Clang。Makefile 默认走本地代理 `http://127.0.0.1:7897`，没有代理时：

```bash
make http_proxy= https_proxy= ALL_PROXY=
```

## 命令

```bash
make          # 配置并编译到 build/editor
make run      # 编译后启动；传参 make run ARGS=src/app.cpp
make format   # clang-format src/
make clean    # 删除 build/
```

无参数启动打开当前目录。也可直接打开文件或目录：

```bash
./build/editor src/app.cpp
./build/editor .
```

## 配置

`editor.conf` 会复制到 `build/`。路径相对 **build 目录**：

```
font = ../assets/fonts/jetbrains-mono-regular.ttf
```

仓库里还有 `jetbrains-mono-medium.ttf`。中文回退系统字体（PingFang 等）。

字号默认 16pt，范围 10–40。状态栏 `- 16 +` 可调，也支持 `Cmd+=` / `Cmd+-` / `Cmd+0`。

日志写在 `logs/`。

## 快捷键

| 按键 | 作用 |
|---|---|
| `Cmd+O` | 打开文件夹 |
| `Cmd+S` | 保存 |
| `Cmd+J` / `` Cmd+` `` / `F12` | 打开或关闭终端 |
| `Cmd+=` / `Cmd+-` / `Cmd+0` | 放大 / 缩小 / 重置字号 |
| `Esc` | 从终端回到编辑器 |

侧栏与终端面板可拖分隔条。空侧栏点击也会打开文件夹。
