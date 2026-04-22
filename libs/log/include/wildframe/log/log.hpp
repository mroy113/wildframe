#pragma once

/// \file
/// Wildframe logging entry point (S0-14, NFR-5 auditability).
///
/// Exposes the global logger registry plus one pre-registered
/// `Logger` handle per module tag in `docs/STYLE.md` §4.5. Callers
/// use the handle's member functions for `info` / `warn` / `error` /
/// `critical` (the function API is spdlog's idiomatic style); only
/// `trace` / `debug` go through the `WF_TRACE` / `WF_DEBUG` macros
/// so `SPDLOG_ACTIVE_LEVEL` can strip them from release binaries
/// (STYLE.md §4.2).
///
/// \code
///   #include <wildframe/log/log.hpp>
///   namespace log = wildframe::log;
///
///   log::detect.info("loaded model in {} ms", elapsed_ms);
///   log::detect.warn("skipped frame, confidence {} below threshold",
///                    score);
///   WF_TRACE(log::detect, "pre-NMS boxes={}", n);
/// \endcode

// Compile-time level filter per `docs/STYLE.md` §4.2. Must precede
// the spdlog include, since spdlog reads `SPDLOG_ACTIVE_LEVEL` when
// it expands the `SPDLOG_LOGGER_*` macros used by `WF_TRACE` /
// `WF_DEBUG` below.
#ifndef SPDLOG_ACTIVE_LEVEL
#ifdef NDEBUG
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_DEBUG
#else
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#endif
#endif

#include <spdlog/spdlog.h>

#include <cstdint>
#include <filesystem>
#include <string_view>
#include <utility>
#include <vector>

namespace wildframe::log {

/// Threshold applied to every registered logger at `init()`. Mirrors
/// `spdlog::level::level_enum` but kept in our namespace so callers
/// do not need to spell `spdlog::level::*`.
enum class Level : std::uint8_t {
  Trace,
  Debug,
  Info,
  Warn,
  Error,
  Critical,
  Off,
};

/// `init()` input. The orchestrator builds one of these from the
/// TOML config (`log_path`, `log_level`) and passes it in.
struct Config {
  /// Threshold applied to every module logger (`docs/STYLE.md` §4.2).
  Level level = Level::Info;

  /// Daily-rotating file sink path. Empty disables the file sink.
  /// Default per `docs/STYLE.md` §4.3 is
  /// `~/Library/Logs/Wildframe/wildframe.log`; the caller resolves
  /// `~` before passing the path in.
  std::filesystem::path log_path;

  /// Rotated file retention. Default 14 per `docs/STYLE.md` §4.3.
  int max_rotated_files = 14;

  /// Stdout sink toggle. On for developers running from a terminal;
  /// tests disable it to keep `ctest` output quiet.
  bool enable_stdout = true;

  /// Extra sinks merged in alongside stdout/file. Tests use this to
  /// hook in `spdlog::sinks::ostream_sink_mt` for line capture.
  std::vector<spdlog::sink_ptr> extra_sinks;
};

class Logger;

namespace detail {

/// Resolve a handle to its registered spdlog logger. Terminates if
/// the tag is not registered: a missing tag means either
/// `wildframe::log::init()` has not run or the handle's tag is not in
/// `docs/STYLE.md` §4.5 — both programming errors, not runtime
/// conditions.
[[nodiscard]] spdlog::logger* native(const Logger& handle) noexcept;

}  // namespace detail

/// Module logger handle. One constexpr instance per `docs/STYLE.md`
/// §4.5 tag is exposed at namespace scope; callers invoke the member
/// functions directly. The handle itself is stateless — it carries
/// just the tag and resolves to the registered `spdlog::logger` on
/// each call.
class Logger {
 public:
  constexpr explicit Logger(std::string_view tag) noexcept : tag_(tag) {}

  [[nodiscard]] constexpr std::string_view tag() const noexcept { return tag_; }

  // `cppcoreguidelines-missing-std-forward` does not see through
  // parameter-pack expansion of `std::forward<Args>(args)...`
  // (confirmed false positive: each pack element *is* forwarded).
  // Suppress only the affected checker around the four member
  // templates below.
  // NOLINTBEGIN(cppcoreguidelines-missing-std-forward)
  template <class... Args>
  void info(spdlog::format_string_t<Args...> fmt, Args&&... args) const {
    detail::native(*this)->info(fmt, std::forward<Args>(args)...);
  }

  template <class... Args>
  void warn(spdlog::format_string_t<Args...> fmt, Args&&... args) const {
    detail::native(*this)->warn(fmt, std::forward<Args>(args)...);
  }

  template <class... Args>
  void error(spdlog::format_string_t<Args...> fmt, Args&&... args) const {
    detail::native(*this)->error(fmt, std::forward<Args>(args)...);
  }

  template <class... Args>
  void critical(spdlog::format_string_t<Args...> fmt, Args&&... args) const {
    detail::native(*this)->critical(fmt, std::forward<Args>(args)...);
  }
  // NOLINTEND(cppcoreguidelines-missing-std-forward)

 private:
  std::string_view tag_;
};

/// Initialize the global logger registry. Registers one logger per
/// module tag listed in `docs/STYLE.md` §4.5, all sharing the sinks
/// in `cfg`. Idempotent: a second call drops the prior loggers and
/// re-creates them against the new config.
///
/// Throws `spdlog::spdlog_ex` if the file sink cannot be opened. The
/// orchestrator translates that to a Wildframe error type at the
/// boundary (`docs/STYLE.md` §3.1).
void init(const Config& cfg);

/// Drop every Wildframe-registered logger. Production code rarely
/// needs this; tests call it between cases to keep state isolated.
void shutdown() noexcept;

// One handle per `docs/STYLE.md` §4.5 tag. `inline constexpr` gives
// external linkage with ODR safety — every translation unit sees the
// same object.
inline constexpr Logger ingest{"ingest"};
inline constexpr Logger raw{"raw"};
inline constexpr Logger detect{"detect"};
inline constexpr Logger focus{"focus"};
inline constexpr Logger metadata{"metadata"};
inline constexpr Logger orchestrator{"orchestrator"};
inline constexpr Logger gui{"gui"};

}  // namespace wildframe::log

// `trace` and `debug` are the only two levels that go through a
// preprocessor macro, per `docs/STYLE.md` §4.5. spdlog's
// `SPDLOG_ACTIVE_LEVEL` compile-time strip only works at the macro
// layer; a plain function call would always compile and keep
// trace-level format cost in release binaries. That is the single
// reason `cppcoreguidelines-macro-usage` is suppressed here.
// NOLINTBEGIN(cppcoreguidelines-macro-usage)
#define WF_TRACE(handle, ...) \
  SPDLOG_LOGGER_TRACE(::wildframe::log::detail::native(handle), __VA_ARGS__)
#define WF_DEBUG(handle, ...) \
  SPDLOG_LOGGER_DEBUG(::wildframe::log::detail::native(handle), __VA_ARGS__)
// NOLINTEND(cppcoreguidelines-macro-usage)
