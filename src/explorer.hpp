#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace sdl {

struct TreeRow {
    std::filesystem::path path;
    std::string name;
    int depth = 0;
    bool is_dir = false;
    bool expanded = false;
    bool active = false;
};

class Explorer {
public:
    Explorer() = default;
    explicit Explorer(std::filesystem::path root);

    void set_root(std::filesystem::path root);
    void set_active(const std::filesystem::path& file);
    void reveal(const std::filesystem::path& file);
    void toggle(int visible_index);

    const std::filesystem::path& root() const { return root_; }
    const std::filesystem::path& active() const { return active_; }
    const std::vector<TreeRow>& rows() const { return rows_; }

private:
    struct Node {
        std::filesystem::path path;
        std::string name;
        bool is_dir = false;
        bool expanded = false;
        bool loaded = false;
        std::vector<Node> children;
    };

    static bool ignored(const std::string& name);
    void load_children(Node& node) const;
    void load_expanded(Node& node) const;
    Node* find(Node& node, const std::filesystem::path& path);
    void expand_to(const std::filesystem::path& file);
    void rebuild();
    void walk(const Node& node, int depth);

    std::filesystem::path root_;
    std::filesystem::path active_;
    Node root_node_;
    std::vector<TreeRow> rows_;
};

}  // namespace sdl
