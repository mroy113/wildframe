#include "wildframe/ingest/enumerate.hpp"

#include <gtest/gtest.h>
#include <spdlog/common.h>

#include <array>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iterator>
#include <string>
#include <system_error>

#include "wildframe/ingest/image_job.hpp"
#include "wildframe/ingest/ingest_error.hpp"
#include "wildframe/log/log.hpp"

namespace {

// --- Test infrastructure ---------------------------------------------------
//
// `Enumerate` emits `warn`-level log lines on per-entry skips. Without an
// initialized sink, `log::ingest.warn(...)` would terminate in
// `detail::native`. The test fixture sets up a silent logger (no stdout,
// no file sink, no extra sinks) shared by every case.

class EnumerateTest : public ::testing::Test {
 protected:
  void SetUp() override {
    wildframe::log::Config cfg;
    cfg.enable_stdout = false;
    cfg.level = spdlog::level::warn;
    wildframe::log::Init(cfg);

    root_ = std::filesystem::path{testing::TempDir()} /
            ::testing::UnitTest::GetInstance()->current_test_info()->name();
    std::filesystem::remove_all(root_);
    std::filesystem::create_directories(root_);
  }

  void TearDown() override {
    wildframe::log::Shutdown();
    std::error_code ignored;
    std::filesystem::remove_all(root_, ignored);
  }

  // CR3 with a valid ftyp/crx header plus trailing junk. Sufficient for
  // the magic-bytes check; we never decode.
  void WriteCr3(const std::filesystem::path& relative) const {
    const std::filesystem::path full = root_ / relative;
    std::filesystem::create_directories(full.parent_path());
    std::ofstream stream(full, std::ios::binary);
    static constexpr std::array<char, 16> kHeader = {
        0x00, 0x00, 0x00, 0x18, 'f',  't',  'y',  'p',
        'c',  'r',  'x',  ' ',  0x00, 0x00, 0x00, 0x01,
    };
    stream.write(kHeader.data(), kHeader.size());
  }

  // A file with a .CR3 extension but the wrong first 12 bytes (M1-03:
  // must be skipped with a `warn` log).
  void WriteFakeCr3(const std::filesystem::path& relative) const {
    const std::filesystem::path full = root_ / relative;
    std::filesystem::create_directories(full.parent_path());
    std::ofstream stream(full, std::ios::binary);
    const std::string junk = "not a real cr3 header";
    stream.write(junk.data(), static_cast<std::streamsize>(junk.size()));
  }

  void WriteNonCr3(const std::filesystem::path& relative) const {
    const std::filesystem::path full = root_ / relative;
    std::filesystem::create_directories(full.parent_path());
    std::ofstream stream(full, std::ios::binary);
    stream << "hello";
  }

  std::filesystem::path root_;
};

// --- Empty / malformed input ---------------------------------------------

TEST_F(EnumerateTest, EmptyDirectoryReturnsEmptyVector) {
  const auto jobs = wildframe::ingest::Enumerate(root_);
  EXPECT_TRUE(jobs.empty());
}

TEST_F(EnumerateTest, NonexistentDirectoryThrowsIngestError) {
  const std::filesystem::path missing = root_ / "does-not-exist";
  EXPECT_THROW(
      { (void)wildframe::ingest::Enumerate(missing); },
      wildframe::ingest::IngestError);
}

TEST_F(EnumerateTest, FilePathThrowsIngestError) {
  const std::filesystem::path file = root_ / "a.CR3";
  WriteCr3("a.CR3");
  EXPECT_THROW(
      { (void)wildframe::ingest::Enumerate(file); },
      wildframe::ingest::IngestError);
}

TEST_F(EnumerateTest, NegativeMaxDepthThrowsIngestError) {
  EXPECT_THROW(
      { (void)wildframe::ingest::Enumerate(root_, -1); },
      wildframe::ingest::IngestError);
}

// --- CR3 vs non-CR3 filtering --------------------------------------------

TEST_F(EnumerateTest, CR3FilesEmittedNonCR3Skipped) {
  WriteCr3("a.CR3");
  WriteCr3("b.CR3");
  WriteNonCr3("readme.txt");
  WriteNonCr3("sidecar.xmp");

  const auto jobs = wildframe::ingest::Enumerate(root_);
  ASSERT_EQ(jobs.size(), 2U);
  for (const auto& job : jobs) {
    EXPECT_EQ(job.path.extension(), ".CR3");
    EXPECT_EQ(job.format, wildframe::ingest::Format::kCr3);
    ASSERT_TRUE(job.size_bytes.has_value());
    // Second `has_value()` check satisfies
    // `bugprone-unchecked-optional-access`; the dataflow analysis
    // cannot model gtest's `ASSERT_*`-aborts-the-caller contract, so
    // we pair the assertion with a plain `if` to make the access
    // self-evidently safe. See docs/STYLE.md §2.13.
    if (job.size_bytes.has_value()) {
      EXPECT_GT(*job.size_bytes, 0U);
    }
  }
}

TEST_F(EnumerateTest, LowercaseExtensionAccepted) {
  WriteCr3("a.cr3");
  const auto jobs = wildframe::ingest::Enumerate(root_);
  ASSERT_EQ(jobs.size(), 1U);
}

TEST_F(EnumerateTest, FakeCR3HeaderIsSkipped) {
  WriteCr3("good.CR3");
  WriteFakeCr3("bad.CR3");

  const auto jobs = wildframe::ingest::Enumerate(root_);
  ASSERT_EQ(jobs.size(), 1U);
  EXPECT_EQ(jobs.front().path.filename(), "good.CR3");
}

// --- Determinism ---------------------------------------------------------

TEST_F(EnumerateTest, ResultIsSortedByPath) {
  WriteCr3("zebra.CR3");
  WriteCr3("alpha.CR3");
  WriteCr3("mango.CR3");

  const auto jobs = wildframe::ingest::Enumerate(root_);
  ASSERT_EQ(jobs.size(), 3U);
  for (auto current = std::next(jobs.begin()); current != jobs.end();
       ++current) {
    EXPECT_LT(std::prev(current)->path, current->path);
  }
}

// --- Depth handling ------------------------------------------------------

TEST_F(EnumerateTest, DefaultDepthOneDescendsOneLevel) {
  WriteCr3("top.CR3");
  WriteCr3("sub/inside.CR3");
  WriteCr3("sub/deeper/buried.CR3");

  const auto jobs = wildframe::ingest::Enumerate(root_);
  ASSERT_EQ(jobs.size(), 2U);
  EXPECT_EQ(jobs.front().path.filename(), "inside.CR3");
  EXPECT_EQ(jobs.back().path.filename(), "top.CR3");
}

TEST_F(EnumerateTest, MaxDepthZeroStaysInRoot) {
  WriteCr3("top.CR3");
  WriteCr3("sub/inside.CR3");

  const auto jobs = wildframe::ingest::Enumerate(root_, 0);
  ASSERT_EQ(jobs.size(), 1U);
  EXPECT_EQ(jobs.front().path.filename(), "top.CR3");
}

TEST_F(EnumerateTest, DeepTreeReachedWithLargerDepth) {
  WriteCr3("a/b/c/d.CR3");

  EXPECT_EQ(wildframe::ingest::Enumerate(root_, 2).size(), 0U);
  EXPECT_EQ(wildframe::ingest::Enumerate(root_, 3).size(), 1U);
}

// --- Symlink skip --------------------------------------------------------

TEST_F(EnumerateTest, SymlinksAreSkipped) {
  WriteCr3("real.CR3");

  std::error_code link_error;
  std::filesystem::create_symlink(root_ / "real.CR3", root_ / "link.CR3",
                                  link_error);
  if (link_error) {
    GTEST_SKIP() << "symlink creation unsupported: " << link_error.message();
  }

  const auto jobs = wildframe::ingest::Enumerate(root_);
  ASSERT_EQ(jobs.size(), 1U);
  EXPECT_EQ(jobs.front().path.filename(), "real.CR3");
}

// --- Canonicalization ----------------------------------------------------

TEST_F(EnumerateTest, PathsAreAbsolute) {
  WriteCr3("a.CR3");
  const auto jobs = wildframe::ingest::Enumerate(
      std::filesystem::path{"."} / std::filesystem::relative(root_));
  ASSERT_EQ(jobs.size(), 1U);
  EXPECT_TRUE(jobs.front().path.is_absolute()) << jobs.front().path.string();
}

// --- S0-12 fixture coverage ---------------------------------------------

TEST_F(EnumerateTest, RealCR3FixturesAreRecognized) {
  const std::filesystem::path fixtures{WILDFRAME_FIXTURES_DIR};
  if (!std::filesystem::exists(fixtures)) {
    GTEST_SKIP() << "S0-12 fixtures unavailable: " << fixtures;
  }

  const auto jobs = wildframe::ingest::Enumerate(fixtures);
  // S0-12 currently pins three CR3 fixtures; tests should hold as long
  // as the count is non-zero — S0-12b will add two more when acquired.
  ASSERT_FALSE(jobs.empty());
  for (const auto& job : jobs) {
    EXPECT_EQ(job.format, wildframe::ingest::Format::kCr3);
  }
}

}  // namespace
