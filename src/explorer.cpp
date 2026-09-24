#include "explorer.hpp"

#include <algorithm>
#include <system_error>

namespace sdl {

namespace {

std::filesystem::path normalized(std::filesystem::path path) {
  std::error_code ec;
  auto abs = std::filesystem::absolute(path, ec);
  if (ec) {
    abs = std::move(path);
  }
  return abs.lexically_normal();
}

}  // namespace

Explorer::Explorer(std::filesystem::path root) { set_root(std::move(root)); }

bool Explorer::ignored(const std::string& name) {
  return name == ".git" || name == "node_modules" || name == "__pycache__" ||
         name == ".DS_Store";
}

void Explorer::set_root(std::filesystem::path root) {
  root_ = normalized(std::move(root));
  root_node_.path = root_;
  root_node_.name = root_.filename().string();
  if (root_node_.name.empty()) {
    root_node_.name = root_.string();
  }
  root_node_.is_dir = true;
  root_node_.expanded = true;
  root_node_.loaded = false;
  root_node_.children.clear();
  active_.clear();
  rebuild();
}

void Explorer::set_active(const std::filesystem::path& file) {
  active_ = normalized(file);
  rebuild();
}

void Explorer::reveal(const std::filesystem::path& file) {
  expand_to(normalized(file));
  set_active(file);
}

void Explorer::toggle(int visible_index) {
  if (visible_index < 0 || visible_index >= static_cast<int>(rows_.size())) {
    return;
  }
  const auto path = rows_[static_cast<std::size_t>(visible_index)].path;
  Node* node = find(root_node_, path);
  if (!node || !node->is_dir) {
    return;
  }
  node->expanded = !node->expanded;
  if (node->expanded) {
    node->loaded = false;
    load_children(*node);
  }
  rebuild();
}

void Explorer::load_children(Node& node) const {
  // Lazy: only directories, and only once until toggle() clears loaded.
  if (!node.is_dir || node.loaded) {
    return;
  }

  node.loaded = true;
  node.children.clear();

  // skip_permission_denied keeps one unreadable entry from aborting the walk.
  std::error_code ec;
  for (const auto& entry : std::filesystem::directory_iterator(
           node.path,
           std::filesystem::directory_options::skip_permission_denied, ec)) {
    if (ec) {
      break;
    }

    const auto name = entry.path().filename().string();
    if (name == "." || name == ".." || ignored(name)) {
      continue;
    }

    std::error_code type_ec;
    Node child;
    child.path = normalized(entry.path());
    child.name = name;

    // Prefer the iterator's type; fall back to a fresh status check.
    child.is_dir = entry.is_directory(type_ec);
    if (type_ec) {
      child.is_dir = std::filesystem::is_directory(child.path, type_ec);
    }
    if (type_ec) {
      continue;
    }

    node.children.push_back(std::move(child));
  }

  // Directories first, then files; each group sorted by name.
  std::sort(node.children.begin(), node.children.end(),
            [](const Node& a, const Node& b) {
              if (a.is_dir != b.is_dir) {
                return a.is_dir;
              }
              return a.name < b.name;
            });
}

Explorer::Node* Explorer::find(Node& node, const std::filesystem::path& path) {
  if (node.path == path ||
      node.path.lexically_normal() == path.lexically_normal()) {
    return &node;
  }
  std::error_code a_ec;
  std::error_code b_ec;
  std::error_code eq_ec;
  if (std::filesystem::exists(node.path, a_ec) && !a_ec &&
      std::filesystem::exists(path, b_ec) && !b_ec &&
      std::filesystem::equivalent(node.path, path, eq_ec) && !eq_ec) {
    return &node;
  }
  for (Node& child : node.children) {
    if (Node* found = find(child, path)) {
      return found;
    }
  }
  return nullptr;
}

void Explorer::load_expanded(Node& node) const {
  if (!node.is_dir || !node.expanded) {
    return;
  }
  load_children(node);
  for (Node& child : node.children) {
    load_expanded(child);
  }
}

void Explorer::expand_to(const std::filesystem::path& file) {
  std::error_code ec;
  const auto relative = std::filesystem::relative(file, root_, ec);
  if (ec || relative.empty() || relative.native().starts_with("..")) {
    return;
  }
  Node* node = &root_node_;
  node->expanded = true;
  load_children(*node);
  std::filesystem::path cursor = root_;
  for (const auto& part : relative) {
    if (part == "." || part.empty()) {
      continue;
    }
    cursor /= part;
    const auto want = normalized(cursor);
    Node* next = nullptr;
    for (Node& child : node->children) {
      if (child.path == want) {
        next = &child;
        break;
      }
    }
    if (!next) {
      return;
    }
    if (next->is_dir) {
      next->expanded = true;
      load_children(*next);
    }
    node = next;
  }
}

void Explorer::rebuild() {
  load_expanded(root_node_);
  rows_.clear();
  walk(root_node_, 0);
}

void Explorer::walk(const Node& node, int depth) {
  TreeRow row;
  row.path = node.path;
  row.name = node.name;
  row.depth = depth;
  row.is_dir = node.is_dir;
  row.expanded = node.expanded;
  row.active = !node.is_dir && node.path == active_;
  rows_.push_back(std::move(row));
  if (!node.is_dir || !node.expanded) {
    return;
  }
  for (const Node& child : node.children) {
    walk(child, depth + 1);
  }
}

}  // namespace sdl
