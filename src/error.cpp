#include "error.hpp"

#include <stdexcept>

#include "log.hpp"

namespace sdl {

[[noreturn]] void throw_error(const char* what) {
  const std::string msg = last_error(what);
  LOG_ERROR("{}", msg);
  throw std::runtime_error(msg);
}

}  // namespace sdl
