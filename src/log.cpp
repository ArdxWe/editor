#include "log.hpp"

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <chrono>
#include <ctime>
#include <filesystem>
#include <memory>
#include <vector>

namespace sdl {

void init_logging() {
  std::error_code ec;
  std::filesystem::create_directories("logs", ec);

  const auto now = std::chrono::system_clock::now();
  const std::time_t t = std::chrono::system_clock::to_time_t(now);
  std::tm local{};
#if defined(_WIN32)
  localtime_s(&local, &t);
#else
  localtime_r(&t, &local);
#endif
  char stamp[32];
  std::strftime(stamp, sizeof(stamp), "%Y-%m-%d-%H-%M-%S", &local);
  const std::filesystem::path file =
      std::filesystem::path("logs") / (std::string(stamp) + ".log");

  std::vector<spdlog::sink_ptr> sinks;
  auto console = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  console->set_level(spdlog::level::debug);
  sinks.push_back(std::move(console));

  if (!ec) {
    auto rotating = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
        file.string(), true);
    rotating->set_level(spdlog::level::debug);
    sinks.push_back(std::move(rotating));
  }

  auto logger =
      std::make_shared<spdlog::logger>("editor", sinks.begin(), sinks.end());
  logger->set_level(spdlog::level::debug);
  logger->flush_on(spdlog::level::debug);
  logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %s:%# %v");
  spdlog::set_default_logger(std::move(logger));

  LOG_INFO("logging to {}", file.string());
}

}  // namespace sdl
