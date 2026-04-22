#include "wildframe/log/log.hpp"

#include <gtest/gtest.h>
#include <spdlog/common.h>
// misc-include-cleaner false-positives on this include: we do use
// `spdlog::sinks::ostream_sink_mt` directly below, and the type is
// declared in this header.
#include <spdlog/sinks/ostream_sink.h>  // NOLINT(misc-include-cleaner)
#include <spdlog/spdlog.h>

#include <array>
#include <memory>
#include <regex>
#include <sstream>
#include <string>
#include <utility>

namespace {

constexpr std::array<const char*, 7> kAllModuleTags = {
    "ingest", "raw", "detect", "focus", "metadata", "orchestrator", "gui",
};

struct CaptureSink {
  std::shared_ptr<std::ostringstream> stream;
  std::shared_ptr<spdlog::sinks::ostream_sink_mt> sink;
};

CaptureSink make_capture_sink() {
  auto stream = std::make_shared<std::ostringstream>();
  auto sink = std::make_shared<spdlog::sinks::ostream_sink_mt>(*stream);
  return {.stream = std::move(stream), .sink = std::move(sink)};
}

void init_with_capture(
    const std::shared_ptr<spdlog::sinks::ostream_sink_mt>& sink,
    spdlog::level::level_enum level = spdlog::level::info) {
  wildframe::log::Config cfg;
  cfg.level = level;
  cfg.enable_stdout = false;
  cfg.extra_sinks.push_back(sink);
  wildframe::log::init(cfg);
}

TEST(WildframeLog, RegistersAllModuleTagsFromStyleSpec) {
  auto cap = make_capture_sink();
  init_with_capture(cap.sink);

  for (const auto* tag : kAllModuleTags) {
    EXPECT_NE(spdlog::get(tag), nullptr) << "missing logger for tag: " << tag;
  }

  wildframe::log::shutdown();
}

TEST(WildframeLog, ReinitDropsAndRecreatesLoggers) {
  auto cap = make_capture_sink();
  init_with_capture(cap.sink);
  auto* first = spdlog::get("detect").get();
  ASSERT_NE(first, nullptr);

  auto cap2 = make_capture_sink();
  init_with_capture(cap2.sink);
  auto* second = spdlog::get("detect").get();

  EXPECT_NE(second, nullptr);
  EXPECT_NE(first, second);

  wildframe::log::shutdown();
}

TEST(WildframeLog, ShutdownDropsRegisteredLoggers) {
  auto cap = make_capture_sink();
  init_with_capture(cap.sink);
  ASSERT_NE(spdlog::get("detect"), nullptr);

  wildframe::log::shutdown();

  for (const auto* tag : kAllModuleTags) {
    EXPECT_EQ(spdlog::get(tag), nullptr)
        << "tag still registered after shutdown: " << tag;
  }
}

TEST(WildframeLog, EmittedLineMatchesStylePatternAndTag) {
  auto cap = make_capture_sink();
  init_with_capture(cap.sink);

  wildframe::log::detect.info("test message {}", 42);
  spdlog::get("detect")->flush();

  // Expected per docs/STYLE.md §4.4:
  //   <YYYY-MM-DD HH:MM:SS.mmm> [<level>] [<module>] <message>
  // The ostream sink does not apply terminal color, so %^...%$
  // collapses to the bare level token.
  const std::string out = cap.stream->str();
  static const std::regex kLine{
      R"(\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\.\d{3} \[info\] \[detect\] test message 42)"};
  // readability-implicit-bool-conversion false-positives on GTest's
  // streaming-message expansion here; the actual-output string is
  // worth keeping for regex debugging.
  // NOLINTNEXTLINE(readability-implicit-bool-conversion)
  EXPECT_TRUE(std::regex_search(out, kLine)) << "actual output: " << out;

  wildframe::log::shutdown();
}

TEST(WildframeLog, AllLevelsCompileAndRouteToTaggedLogger) {
  auto cap = make_capture_sink();
  init_with_capture(cap.sink, spdlog::level::trace);

  WF_TRACE(wildframe::log::detect, "t {}", 1);
  WF_DEBUG(wildframe::log::detect, "d {}", 2);
  wildframe::log::detect.info("i {}", 3);
  wildframe::log::detect.warn("w {}", 4);
  wildframe::log::detect.error("e {}", 5);
  wildframe::log::detect.critical("c {}", 6);
  spdlog::get("detect")->flush();

  const std::string out = cap.stream->str();
  // info / warn / error / critical are always emitted past the
  // SPDLOG_ACTIVE_LEVEL filter; trace and debug depend on whether
  // the test was built debug or release. Assert only the always-on
  // four so the test passes under either build.
  EXPECT_NE(out.find("[info] [detect] i 3"), std::string::npos);
  EXPECT_NE(out.find("[warning] [detect] w 4"), std::string::npos);
  EXPECT_NE(out.find("[error] [detect] e 5"), std::string::npos);
  EXPECT_NE(out.find("[critical] [detect] c 6"), std::string::npos);

  wildframe::log::shutdown();
}

TEST(WildframeLog, HandleAndTagAreExposedAtNamespaceScope) {
  EXPECT_EQ(wildframe::log::detect.tag(), "detect");
  EXPECT_EQ(wildframe::log::orchestrator.tag(), "orchestrator");
  EXPECT_EQ(wildframe::log::gui.tag(), "gui");
}

}  // namespace
