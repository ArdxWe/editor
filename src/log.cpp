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
  // Relative to the process cwd (usually the repo root when launched via make).
  std::error_code ec;
  std::filesystem::create_directories("logs", ec);

  // One file per launch: logs/YYYY-MM-DD-HH-MM-SS.log
  const auto now = std::chrono::system_clock::now();
  const std::time_t t = std::chrono::system_clock::to_time_t(now);
  std::tm local{};
  localtime_r(&t, &local);
  char stamp[32];
  std::strftime(stamp, sizeof(stamp), "%Y-%m-%d-%H-%M-%S", &local);
  const std::filesystem::path file =
      std::filesystem::path("logs") / (std::string(stamp) + ".log");

  // Console always; file sink only if the logs/ directory is usable.
  std::vector<spdlog::sink_ptr> sinks;
  auto console = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  console->set_level(spdlog::level::debug);
  sinks.push_back(std::move(console));

  if (!ec) {
    auto file_sink =
        std::make_shared<spdlog::sinks::basic_file_sink_mt>(file.string(), true);
    file_sink->set_level(spdlog::level::debug);
    sinks.push_back(std::move(file_sink));
  }

  auto logger =
      std::make_shared<spdlog::logger>("editor", sinks.begin(), sinks.end());
  logger->set_level(spdlog::level::debug);
  logger->flush_on(spdlog::level::debug);
  // [%s:%#] is source file and line from the LOG_* macros.
  logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %s:%# %v");
  spdlog::set_default_logger(std::move(logger));

  LOG_INFO("logging to {}", file.string());
}

}  // namespace sdl
