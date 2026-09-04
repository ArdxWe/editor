#pragma once

#include <cstddef>
#include <string>

namespace sdl {

inline std::size_t utf8_next(const std::string& s, std::size_t i) {
    if (i >= s.size()) {
        return s.size();
    }
    const unsigned char c = static_cast<unsigned char>(s[i]);
    std::size_t n = 1;
    if ((c & 0xF8) == 0xF0) {
        n = 4;
    } else if ((c & 0xF0) == 0xE0) {
        n = 3;
    } else if ((c & 0xE0) == 0xC0) {
        n = 2;
    }
    if (i + n > s.size()) {
        return s.size();
    }
    return i + n;
}

inline std::size_t utf8_prev(const std::string& s, std::size_t i) {
    if (i == 0) {
        return 0;
    }
    --i;
    while (i > 0 && (static_cast<unsigned char>(s[i]) & 0xC0) == 0x80) {
        --i;
    }
    return i;
}

}  // namespace sdl
