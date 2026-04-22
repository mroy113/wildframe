#include "wildframe/log/log.hpp"

// misc-include-cleaner does not have a mapping for spdlog's public
// symbol surface: it flags the sink headers below as "not used
// directly" even though we instantiate their types here, and flags
// every `spdlog::level::*` / `spdlog::drop_all` / `spdlog::logger` /
// `spdlog::register_logger` reference as lacking a direct include
// even though <spdlog/spdlog.h> is the umbrella header. Scope the
// suppression to this file rather than leaking NOLINTs onto every
// line.
// NOLINTBEGIN(misc-include-cleaner)
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <array>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace wildframe::log {

namespace {

// Pattern from `docs/STYLE.md` §4.4. `%^%l%$` colorizes the level on
// the stdout sink; spdlog strips the color markers on non-tty sinks,
// so the same pattern is used verbatim everywhere.
constexpr std::string_view kPattern = "%Y-%m-%d %H:%M:%S.%e [%^%l%$] [%n] %v";

// Module tags from `docs/STYLE.md` §4.5. Update both files together.
constexpr std::array<std::string_view, 7> kModuleTags = {
    "ingest", "raw", "detect", "focus", "metadata", "orchestrator", "gui",
};

constexpr spdlog::level::level_enum to_spd_level(Level level) noexcept {
  switch (level) {
    case Level::Trace:
      return spdlog::level::trace;
    case Level::Debug:
      return spdlog::level::debug;
    case Level::Info:
      return spdlog::level::info;
    case Level::Warn:
      return spdlog::level::warn;
    case Level::Error:
      return spdlog::level::err;
    case Level::Critical:
      return spdlog::level::critical;
    case Level::Off:
      return spdlog::level::off;
  }
  return spdlog::level::info;
}

}  // namespace

void init(const Config& cfg) {
  spdlog::drop_all();

  std::vector<spdlog::sink_ptr> sinks;
  if (cfg.enable_stdout) {
    sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
  }
  if (!cfg.log_path.empty()) {
    sinks.push_back(std::make_shared<spdlog::sinks::daily_file_sink_mt>(
        cfg.log_path.string(),
        /*rotation_hour=*/0,
        /*rotation_minute=*/0,
        /*truncate=*/false, cfg.max_rotated_files));
  }
  for (const auto& sink : cfg.extra_sinks) {
    sinks.push_back(sink);
  }

  const auto threshold = to_spd_level(cfg.level);
  const std::string pattern{kPattern};
  for (const auto tag : kModuleTags) {
    const std::string name{tag};
    auto logger =
        std::make_shared<spdlog::logger>(name, sinks.begin(), sinks.end());
    logger->set_level(threshold);
    logger->set_pattern(pattern);
    // Flush on warn so an `error` or `critical` line is on disk
    // before the orchestrator writes the corresponding manifest row
    // (see `docs/STYLE.md` §4.7).
    logger->flush_on(spdlog::level::warn);
    spdlog::register_logger(std::move(logger));
  }
}

void shutdown() noexcept { spdlog::drop_all(); }

namespace detail {

// `std::string{tag}` can throw `bad_alloc`; `noexcept` then routes that
// to `std::terminate`, which is the behavior we want on this path
// anyway (OOM during log lookup means the process is already doomed).
// NOLINTNEXTLINE(bugprone-exception-escape)
spdlog::logger* native(const Logger& handle) noexcept {
  const auto tag = handle.tag();
  if (auto logger = spdlog::get(std::string{tag}); logger) {
    return logger.get();
  }
  // A missing tag means either `wildframe::log::init()` has not run
  // or the handle's tag is not in `docs/STYLE.md` §4.5 — both
  // programming errors. Terminate loudly rather than silently log
  // into the void. fputs/fwrite return values are intentionally
  // discarded: we are on the abort path, so there is no recovery.
  (void)std::fputs("wildframe::log: no logger registered for tag '", stderr);
  // tag is a string_view and not guaranteed to be null-terminated,
  // so write the bytes directly.
  (void)std::fwrite(tag.data(), 1, tag.size(), stderr);
  (void)std::fputs(
      "'. Did wildframe::log::init() run, and is the handle tag one "
      "of those in docs/STYLE.md \xC2\xA7"
      "4.5?\n",
      stderr);
  std::abort();
}

}  // namespace detail

}  // namespace wildframe::log
// NOLINTEND(misc-include-cleaner)
