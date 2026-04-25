#include "wildframe/metadata/metadata_write_stage.hpp"

#include <gtest/gtest.h>
#include <spdlog/common.h>

#include <chrono>
#include <exiv2/xmp_exiv2.hpp>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iterator>
#include <memory>
#include <string>
#include <system_error>

#include "wildframe/ingest/image_job.hpp"
#include "wildframe/log/log.hpp"
#include "wildframe/orchestrator/pipeline_stage.hpp"

namespace {

class MetadataWriteStageTest : public ::testing::Test {
 protected:
  void SetUp() override {
    wildframe::log::Config cfg;
    cfg.enable_stdout = false;
    cfg.level = spdlog::level::warn;
    wildframe::log::Init(cfg);

    temp_dir_ =
        std::filesystem::temp_directory_path() /
        ("wildframe-metadata-write-stage-test-" +
         std::to_string(::testing::UnitTest::GetInstance()
                            ->current_test_info()
                            ->result()
                            ->start_timestamp()) +
         "-" + ::testing::UnitTest::GetInstance()->current_test_info()->name());
    std::filesystem::create_directories(temp_dir_);
  }

  void TearDown() override {
    std::error_code ignored;
    std::filesystem::remove_all(temp_dir_, ignored);
    wildframe::log::Shutdown();
  }

  [[nodiscard]] std::filesystem::path RawFixture(const std::string& name) {
    const auto path = temp_dir_ / name;
    std::ofstream{path}.close();
    return path;
  }

  std::filesystem::path temp_dir_;
};

TEST_F(MetadataWriteStageTest, WritesSidecarForJob) {
  constexpr auto instant =
      std::chrono::sys_days{std::chrono::year{2026} / std::chrono::April /
                            std::chrono::day{21}} +
      std::chrono::hours{18} + std::chrono::minutes{42} +
      std::chrono::seconds{3};

  auto stage = wildframe::metadata::MakeMetadataWriteStage(instant);
  ASSERT_NE(stage, nullptr);
  EXPECT_EQ(stage->Name(), "metadata_write");

  const auto raw = RawFixture("IMG_0010.CR3");
  wildframe::orchestrator::StageContext context{
      .job = wildframe::ingest::ImageJob{.path = raw},
  };

  (void)stage->Process(context);

  const auto sidecar = std::filesystem::path{raw}.concat(".xmp");
  ASSERT_TRUE(std::filesystem::exists(sidecar));

  std::ifstream in{sidecar, std::ios::binary};
  const std::string packet{std::istreambuf_iterator<char>{in},
                           std::istreambuf_iterator<char>{}};
  Exiv2::XmpData xmp;
  ASSERT_EQ(Exiv2::XmpParser::decode(xmp, packet), 0);

  const auto iter =
      xmp.findKey(Exiv2::XmpKey{"Xmp.wildframe_provenance.analysis_timestamp"});
  ASSERT_NE(iter, xmp.end());
  if (iter != xmp.end()) {
    EXPECT_EQ(iter->toString(), "2026-04-21T18:42:03Z");
  }
}

TEST_F(MetadataWriteStageTest, CapturesTimestampAtConstruction) {
  // Two jobs driven through one stage instance must share the
  // construction-time timestamp per METADATA.md §7.2 (all sidecars in
  // a run share the same `analysis_timestamp`).
  constexpr auto instant =
      std::chrono::sys_days{std::chrono::year{2027} / std::chrono::January /
                            std::chrono::day{2}} +
      std::chrono::hours{3} + std::chrono::minutes{4} + std::chrono::seconds{5};
  auto stage = wildframe::metadata::MakeMetadataWriteStage(instant);

  const auto a = RawFixture("a.CR3");
  const auto b = RawFixture("b.CR3");

  wildframe::orchestrator::StageContext ctx_a{
      .job = wildframe::ingest::ImageJob{.path = a},
  };
  wildframe::orchestrator::StageContext ctx_b{
      .job = wildframe::ingest::ImageJob{.path = b},
  };
  (void)stage->Process(ctx_a);
  (void)stage->Process(ctx_b);

  auto read_stamp = [](const std::filesystem::path& sidecar) {
    std::ifstream in{sidecar, std::ios::binary};
    const std::string packet{std::istreambuf_iterator<char>{in},
                             std::istreambuf_iterator<char>{}};
    Exiv2::XmpData xmp;
    EXPECT_EQ(Exiv2::XmpParser::decode(xmp, packet), 0);
    const auto iter = xmp.findKey(
        Exiv2::XmpKey{"Xmp.wildframe_provenance.analysis_timestamp"});
    if (iter == xmp.end()) {
      return std::string{};
    }
    return iter->toString();
  };

  const auto stamp_a = read_stamp(std::filesystem::path{a}.concat(".xmp"));
  const auto stamp_b = read_stamp(std::filesystem::path{b}.concat(".xmp"));
  EXPECT_EQ(stamp_a, "2027-01-02T03:04:05Z");
  EXPECT_EQ(stamp_b, stamp_a);
}

}  // namespace
