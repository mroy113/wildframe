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
#include <exception>
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

  const std::string pattern{kPattern};
  for (const auto tag : kModuleTags) {
    const std::string name{tag};
    auto logger =
        std::make_shared<spdlog::logger>(name, sinks.begin(), sinks.end());
    logger->set_level(cfg.level);
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

spdlog::logger* native(const Logger& handle) noexcept {
  const auto tag = handle.tag();

  // `spdlog::get` takes a `const std::string&`, so the lookup
  // allocates a short-lived key from our `string_view` tag. If
  // construction throws (OOM, or theoretically `length_error` on an
  // oversized tag), write a diagnostic and abort — there is no
  // useful recovery from a failed log-path lookup, and silently
  // returning null would violate the `noexcept` contract callers
  // rely on. fputs/fwrite return values are intentionally discarded
  // on this abort path.
  try {
    if (auto logger = spdlog::get(std::string{tag}); logger) {
      return logger.get();
    }
  } catch (const std::exception& e) {
    (void)std::fputs("wildframe::log: exception during logger lookup for tag '",
                     stderr);
    (void)std::fwrite(tag.data(), 1, tag.size(), stderr);
    (void)std::fputs("': ", stderr);
    (void)std::fputs(e.what(), stderr);
    (void)std::fputs("\n", stderr);
    std::abort();
  }

  // Reaching here means `spdlog::get` returned null: either
  // `wildframe::log::init()` has not run, or the handle's tag is not
  // in `docs/STYLE.md` §4.5 — both programming errors. Terminate
  // loudly rather than silently log into the void.
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
