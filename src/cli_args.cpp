#include "cli_args.hpp"

#include <string>
#include <string_view>

namespace wildframe::cli {

namespace {

constexpr std::string_view kUsageText =
    "Usage: wildframe [--config <path>] <directory>\n"
    "       wildframe --help | --version\n"
    "\n"
    "Options:\n"
    "  --config <path>  TOML config file (see docs/CONFIG.md). Overrides\n"
    "                   the XDG / macOS user-config lookup.\n"
    "  --help, -h       Print this message and exit.\n"
    "  --version, -v    Print the Wildframe version and exit.\n"
    "\n"
    "Positional:\n"
    "  <directory>      Directory of CR3 RAW files to analyze. Required\n"
    "                   unless --help / --version is given.\n"
    "\n"
    "Exit codes:\n"
    "  0  success\n"
    "  2  usage error (bad command-line)\n"
    "  3  configuration error (bad config file)\n";

// WILDFRAME_VERSION is injected by src/CMakeLists.txt so the string
// stays in one place (the project() declaration in the top-level
// CMakeLists.txt).
#ifndef WILDFRAME_VERSION
#define WILDFRAME_VERSION "0.0.0"
#endif
constexpr std::string_view kVersionText = "wildframe " WILDFRAME_VERSION "\n";

}  // namespace

std::string_view UsageText() noexcept { return kUsageText; }

std::string_view VersionText() noexcept { return kVersionText; }

ParsedArgs ParseArgs(int argc, const char* const* argv) {
  ParsedArgs result{};

  // argv[0] is the program name; iterate argv[1..argc) with pointer
  // walking so clang-tidy's unchecked-container-access check stays
  // happy without a gsl::span wrapper. Pointer arithmetic is allowed
  // repo-wide per .clang-tidy (§NFR-8 "preference-based disables"
  // block).
  if (argc < 1) {
    // Pathological — main() guarantees argc >= 1, but guard anyway so
    // `argv + argc` below is defined.
    throw UsageError{"empty argv"};
  }
  const char* const* cursor = argv + 1;
  const char* const* const end = argv + argc;

  bool saw_directory = false;

  while (cursor != end) {
    const std::string_view arg{*cursor};
    ++cursor;

    if (arg == "--help" || arg == "-h") {
      result.action = Action::kShowHelp;
      return result;
    }
    if (arg == "--version" || arg == "-v") {
      result.action = Action::kShowVersion;
      return result;
    }
    if (arg == "--config") {
      if (cursor == end) {
        throw UsageError{"--config requires a path argument"};
      }
      result.config_path = std::filesystem::path{*cursor};
      ++cursor;
      continue;
    }
    if (arg.starts_with("--")) {
      throw UsageError{"unrecognized option: " + std::string{arg}};
    }

    // Positional.
    if (saw_directory) {
      throw UsageError{"unexpected extra argument: " + std::string{arg}};
    }
    result.directory = std::filesystem::path{arg};
    saw_directory = true;
  }

  if (!saw_directory) {
    throw UsageError{"missing required <directory> argument"};
  }

  return result;
}

}  // namespace wildframe::cli
