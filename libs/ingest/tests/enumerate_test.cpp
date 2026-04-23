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

namespace fs = std::filesystem;
namespace wfi = wildframe::ingest;

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

    root_ = fs::path{testing::TempDir()} /
            ::testing::UnitTest::GetInstance()->current_test_info()->name();
    fs::remove_all(root_);
    fs::create_directories(root_);
  }

  void TearDown() override {
    wildframe::log::Shutdown();
    std::error_code ignored;
    fs::remove_all(root_, ignored);
  }

  [[nodiscard]] const fs::path& root() const { return root_; }

  // CR3 with a valid ftyp/crx header plus trailing junk. Sufficient for
  // the magic-bytes check; we never decode.
  void WriteCr3(const fs::path& relative) const {
    const fs::path full = root_ / relative;
    fs::create_directories(full.parent_path());
    std::ofstream stream(full, std::ios::binary);
    static constexpr std::array<char, 16> kHeader = {
        0x00, 0x00, 0x00, 0x18, 'f',  't',  'y',  'p',
        'c',  'r',  'x',  ' ',  0x00, 0x00, 0x00, 0x01,
    };
    stream.write(kHeader.data(), kHeader.size());
  }

  // A file with a .CR3 extension but the wrong first 12 bytes (M1-03:
  // must be skipped with a `warn` log).
  void WriteFakeCr3(const fs::path& relative) const {
    const fs::path full = root_ / relative;
    fs::create_directories(full.parent_path());
    std::ofstream stream(full, std::ios::binary);
    const std::string junk = "not a real cr3 header";
    stream.write(junk.data(), static_cast<std::streamsize>(junk.size()));
  }

  void WriteNonCr3(const fs::path& relative) const {
    const fs::path full = root_ / relative;
    fs::create_directories(full.parent_path());
    std::ofstream stream(full, std::ios::binary);
    stream << "hello";
  }

 private:
  fs::path root_;
};

// Assertions factored out of `CR3FilesEmittedNonCR3Skipped` to keep that
// test body under the `readability-function-cognitive-complexity`
// threshold; `ASSERT_*` macros compose fine from a helper because gtest
// only requires the caller to return `void`.
void ExpectValidCr3Job(const wfi::ImageJob& job) {
  EXPECT_EQ(job.path.extension(), ".CR3");
  EXPECT_EQ(job.format, wfi::Format::kCr3);
  ASSERT_TRUE(job.size_bytes.has_value());
  // Second `has_value()` check satisfies `bugprone-unchecked-optional-
  // access`; the dataflow analysis cannot model gtest's
  // `ASSERT_*`-aborts-the-caller contract, so we pair the assertion
  // with a plain `if` to make the access self-evidently safe.
  if (job.size_bytes.has_value()) {
    EXPECT_GT(*job.size_bytes, 0U);
  }
}

// --- Empty / malformed input ---------------------------------------------

TEST_F(EnumerateTest, EmptyDirectoryReturnsEmptyVector) {
  const auto jobs = wfi::Enumerate(root());
  EXPECT_TRUE(jobs.empty());
}

TEST_F(EnumerateTest, NonexistentDirectoryThrowsIngestError) {
  const fs::path missing = root() / "does-not-exist";
  EXPECT_THROW({ (void)wfi::Enumerate(missing); }, wfi::IngestError);
}

TEST_F(EnumerateTest, FilePathThrowsIngestError) {
  const fs::path file = root() / "a.CR3";
  WriteCr3("a.CR3");
  EXPECT_THROW({ (void)wfi::Enumerate(file); }, wfi::IngestError);
}

TEST_F(EnumerateTest, NegativeMaxDepthThrowsIngestError) {
  EXPECT_THROW({ (void)wfi::Enumerate(root(), -1); }, wfi::IngestError);
}

// --- CR3 vs non-CR3 filtering --------------------------------------------

TEST_F(EnumerateTest, CR3FilesEmittedNonCR3Skipped) {
  WriteCr3("a.CR3");
  WriteCr3("b.CR3");
  WriteNonCr3("readme.txt");
  WriteNonCr3("sidecar.xmp");

  const auto jobs = wfi::Enumerate(root());
  ASSERT_EQ(jobs.size(), 2U);
  for (const auto& job : jobs) {
    ExpectValidCr3Job(job);
  }
}

TEST_F(EnumerateTest, LowercaseExtensionAccepted) {
  WriteCr3("a.cr3");
  const auto jobs = wfi::Enumerate(root());
  ASSERT_EQ(jobs.size(), 1U);
}

TEST_F(EnumerateTest, FakeCR3HeaderIsSkipped) {
  WriteCr3("good.CR3");
  WriteFakeCr3("bad.CR3");

  const auto jobs = wfi::Enumerate(root());
  ASSERT_EQ(jobs.size(), 1U);
  EXPECT_EQ(jobs.front().path.filename(), "good.CR3");
}

// --- Determinism ---------------------------------------------------------

TEST_F(EnumerateTest, ResultIsSortedByPath) {
  WriteCr3("zebra.CR3");
  WriteCr3("alpha.CR3");
  WriteCr3("mango.CR3");

  const auto jobs = wfi::Enumerate(root());
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

  const auto jobs = wfi::Enumerate(root());
  ASSERT_EQ(jobs.size(), 2U);
  EXPECT_EQ(jobs.front().path.filename(), "inside.CR3");
  EXPECT_EQ(jobs.back().path.filename(), "top.CR3");
}

TEST_F(EnumerateTest, MaxDepthZeroStaysInRoot) {
  WriteCr3("top.CR3");
  WriteCr3("sub/inside.CR3");

  const auto jobs = wfi::Enumerate(root(), 0);
  ASSERT_EQ(jobs.size(), 1U);
  EXPECT_EQ(jobs.front().path.filename(), "top.CR3");
}

TEST_F(EnumerateTest, DeepTreeReachedWithLargerDepth) {
  WriteCr3("a/b/c/d.CR3");

  EXPECT_EQ(wfi::Enumerate(root(), 2).size(), 0U);
  EXPECT_EQ(wfi::Enumerate(root(), 3).size(), 1U);
}

// --- Symlink skip --------------------------------------------------------

TEST_F(EnumerateTest, SymlinksAreSkipped) {
  WriteCr3("real.CR3");

  std::error_code link_error;
  fs::create_symlink(root() / "real.CR3", root() / "link.CR3", link_error);
  if (link_error) {
    GTEST_SKIP() << "symlink creation unsupported: " << link_error.message();
  }

  const auto jobs = wfi::Enumerate(root());
  ASSERT_EQ(jobs.size(), 1U);
  EXPECT_EQ(jobs.front().path.filename(), "real.CR3");
}

// --- Canonicalization ----------------------------------------------------

TEST_F(EnumerateTest, PathsAreAbsolute) {
  WriteCr3("a.CR3");
  const auto jobs = wfi::Enumerate(fs::path{"."} / fs::relative(root()));
  ASSERT_EQ(jobs.size(), 1U);
  EXPECT_TRUE(jobs.front().path.is_absolute()) << jobs.front().path.string();
}

// --- S0-12 fixture coverage ---------------------------------------------

TEST_F(EnumerateTest, RealCR3FixturesAreRecognized) {
  const fs::path fixtures{WILDFRAME_FIXTURES_DIR};
  if (!fs::exists(fixtures)) {
    GTEST_SKIP() << "S0-12 fixtures unavailable: " << fixtures;
  }

  const auto jobs = wfi::Enumerate(fixtures);
  // S0-12 currently pins three CR3 fixtures; tests should hold as long
  // as the count is non-zero — S0-12b will add two more when acquired.
  ASSERT_FALSE(jobs.empty());
  for (const auto& job : jobs) {
    EXPECT_EQ(job.format, wfi::Format::kCr3);
  }
}

}  // namespace
