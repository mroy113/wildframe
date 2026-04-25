#include "wildframe/metadata/sidecar_writer.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <exiv2/properties.hpp>
#include <exiv2/value.hpp>
#include <exiv2/xmp_exiv2.hpp>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iterator>
#include <regex>
#include <string>
#include <system_error>

#include "wildframe/metadata/metadata_error.hpp"
#include "wildframe/metadata/version.hpp"

namespace {

using ::testing::Test;

constexpr auto kFixedInstant =
    std::chrono::sys_days{std::chrono::year{2026} / std::chrono::April /
                          std::chrono::day{21}} +
    std::chrono::hours{18} + std::chrono::minutes{42} + std::chrono::seconds{3};

constexpr const char* kExpectedTimestamp = "2026-04-21T18:42:03Z";

class SidecarWriterTest : public Test {
 protected:
  void SetUp() override {
    temp_dir_ =
        std::filesystem::temp_directory_path() /
        ("wildframe-sidecar-writer-test-" +
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
  }

  [[nodiscard]] std::filesystem::path RawFixture(const std::string& name) {
    const auto path = temp_dir_ / name;
    // LibRaw never opens this file — the writer only needs a stable
    // raw path string. Touch an empty file so filesystem checks in
    // future thickening passes still see something there.
    std::ofstream touch{path};
    touch.close();
    return path;
  }

  static void AddXmpText(Exiv2::XmpData& xmp, const std::string& key,
                         const std::string& text) {
    // Helper used by the seed path — avoids
    // `cppcoreguidelines-pro-bounds-avoid-unchecked-container-access`
    // on `XmpData::operator[]`, mirroring the production writer's
    // internal helper.
    Exiv2::XmpTextValue value;
    value.read(text);
    ASSERT_EQ(xmp.add(Exiv2::XmpKey{key}, &value), 0);
  }

  [[nodiscard]] static Exiv2::XmpData ReadXmp(
      const std::filesystem::path& sidecar) {
    std::ifstream in{sidecar, std::ios::binary};
    EXPECT_TRUE(in) << "could not open " << sidecar;
    const std::string packet{std::istreambuf_iterator<char>{in},
                             std::istreambuf_iterator<char>{}};
    Exiv2::XmpData xmp;
    EXPECT_EQ(Exiv2::XmpParser::decode(xmp, packet), 0);
    return xmp;
  }

  static std::string ReadKey(const Exiv2::XmpData& xmp,
                             const std::string& key) {
    const auto iter = xmp.findKey(Exiv2::XmpKey{key});
    EXPECT_NE(iter, xmp.end()) << "missing key: " << key;
    if (iter == xmp.end()) {
      return {};
    }
    return iter->toString();
  }

  std::filesystem::path temp_dir_;
};

TEST_F(SidecarWriterTest, WritesFiveProvenanceFields) {
  const auto raw = RawFixture("IMG_0001.CR3");
  const auto sidecar = std::filesystem::path{raw}.concat(".xmp");

  wildframe::metadata::WriteProvenanceSidecar(raw, kFixedInstant);

  ASSERT_TRUE(std::filesystem::exists(sidecar));
  const auto xmp = ReadXmp(sidecar);

  EXPECT_EQ(ReadKey(xmp, "Xmp.wildframe_provenance.analysis_timestamp"),
            kExpectedTimestamp);
  EXPECT_EQ(ReadKey(xmp, "Xmp.wildframe_provenance.pipeline_version"),
            std::string{wildframe::metadata::kPipelineVersion});
  EXPECT_EQ(ReadKey(xmp, "Xmp.wildframe_provenance.detector_model_name"),
            "stub");
  EXPECT_EQ(ReadKey(xmp, "Xmp.wildframe_provenance.detector_model_version"),
            "0.0.0");
  EXPECT_EQ(ReadKey(xmp, "Xmp.wildframe_provenance.focus_algorithm_version"),
            "0.0.0");
}

TEST_F(SidecarWriterTest, TimestampIsIso8601Utc) {
  const auto raw = RawFixture("IMG_0002.CR3");
  const auto sidecar = std::filesystem::path{raw}.concat(".xmp");

  wildframe::metadata::WriteProvenanceSidecar(raw, kFixedInstant);

  const auto xmp = ReadXmp(sidecar);
  const auto stamp =
      ReadKey(xmp, "Xmp.wildframe_provenance.analysis_timestamp");

  // METADATA.md §7.2: `YYYY-MM-DDTHH:MM:SSZ`, no offset, Z suffix.
  const std::regex iso8601_utc{R"(^\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}Z$)"};
  EXPECT_TRUE(std::regex_match(stamp, iso8601_utc))
      << "timestamp not ISO-8601 UTC: " << stamp;
}

TEST_F(SidecarWriterTest, RewriteRefreshesExistingSidecar) {
  const auto raw = RawFixture("IMG_0003.CR3");
  const auto sidecar = std::filesystem::path{raw}.concat(".xmp");

  const auto first = kFixedInstant;
  const auto second = kFixedInstant + std::chrono::hours{1};

  wildframe::metadata::WriteProvenanceSidecar(raw, first);
  wildframe::metadata::WriteProvenanceSidecar(raw, second);

  const auto xmp = ReadXmp(sidecar);
  EXPECT_EQ(ReadKey(xmp, "Xmp.wildframe_provenance.analysis_timestamp"),
            "2026-04-21T19:42:03Z");
}

TEST_F(SidecarWriterTest, PreservesForeignNamespaceOnRewrite) {
  const auto raw = RawFixture("IMG_0004.CR3");
  const auto sidecar = std::filesystem::path{raw}.concat(".xmp");

  // Seed the sidecar with an `xmp:CreatorTool` field (plain Text,
  // not a LangAlt) that Wildframe does not own (METADATA.md §1.3).
  // Serialized standalone rather than going through our writer so the
  // preservation guarantee is tested against a foreign writer's state,
  // not a prior Wildframe write.
  constexpr auto kForeignValue = "Adobe Lightroom Classic 13.4";
  {
    Exiv2::XmpData seed;
    AddXmpText(seed, "Xmp.xmp.CreatorTool", kForeignValue);
    std::string packet;
    ASSERT_EQ(Exiv2::XmpParser::encode(packet, seed), 0);
    std::ofstream out{sidecar, std::ios::binary | std::ios::trunc};
    out.write(packet.data(), static_cast<std::streamsize>(packet.size()));
  }

  wildframe::metadata::WriteProvenanceSidecar(raw, kFixedInstant);

  const auto xmp = ReadXmp(sidecar);
  EXPECT_EQ(ReadKey(xmp, "Xmp.xmp.CreatorTool"), kForeignValue);
  EXPECT_EQ(ReadKey(xmp, "Xmp.wildframe_provenance.pipeline_version"),
            std::string{wildframe::metadata::kPipelineVersion});
}

TEST_F(SidecarWriterTest, DropsStaleOwnedFieldsOnRewrite) {
  // Re-writing must not leave stale `wildframe_provenance:*` fields
  // behind from an earlier Wildframe version (METADATA.md §1.3 — we
  // own the namespace in full). Drive both writes through the public
  // entry point so the test exercises the same namespace-registration
  // path as production.
  const auto raw = RawFixture("IMG_0004b.CR3");
  const auto sidecar = std::filesystem::path{raw}.concat(".xmp");

  wildframe::metadata::WriteProvenanceSidecar(raw, kFixedInstant);

  // Hand-edit the sidecar to inject a stale key that an older
  // pipeline version might have written — simulated here by appending
  // a new provenance field the current writer does not produce.
  auto xmp_before = ReadXmp(sidecar);
  AddXmpText(xmp_before, "Xmp.wildframe_provenance.retired_experimental_field",
             "legacy-value");
  {
    std::string packet;
    ASSERT_EQ(Exiv2::XmpParser::encode(packet, xmp_before), 0);
    std::ofstream out{sidecar, std::ios::binary | std::ios::trunc};
    out.write(packet.data(), static_cast<std::streamsize>(packet.size()));
  }

  wildframe::metadata::WriteProvenanceSidecar(
      raw, kFixedInstant + std::chrono::hours{1});

  const auto xmp_after = ReadXmp(sidecar);
  EXPECT_EQ(xmp_after.findKey(Exiv2::XmpKey{
                "Xmp.wildframe_provenance.retired_experimental_field"}),
            xmp_after.end())
      << "stale provenance field survived rewrite";
}

TEST_F(SidecarWriterTest, FailureLeavesExistingSidecarIntact) {
  const auto raw = RawFixture("IMG_0005.CR3");
  const auto sidecar = std::filesystem::path{raw}.concat(".xmp");

  wildframe::metadata::WriteProvenanceSidecar(raw, kFixedInstant);
  const auto original = ReadXmp(sidecar);

  // Replace the sidecar's directory with a file — the next write's
  // same-directory rename will fail, exercising the catch-and-cleanup
  // path without leaving the original sidecar tampered with.
  const auto raw_in_blocked = temp_dir_ / "blocked-parent" / "IMG_0006.CR3";
  std::filesystem::create_directories(raw_in_blocked.parent_path());
  std::ofstream{raw_in_blocked}.close();
  // Pre-seed a sidecar in the blocked directory, then remove write
  // permission so the rename step fails.
  wildframe::metadata::WriteProvenanceSidecar(raw_in_blocked, kFixedInstant);
  const auto blocked_sidecar =
      std::filesystem::path{raw_in_blocked}.concat(".xmp");
  const auto blocked_before = ReadXmp(blocked_sidecar);

  std::filesystem::permissions(
      raw_in_blocked.parent_path(),
      std::filesystem::perms::owner_read | std::filesystem::perms::owner_exec,
      std::filesystem::perm_options::replace);

  EXPECT_THROW(wildframe::metadata::WriteProvenanceSidecar(
                   raw_in_blocked, kFixedInstant + std::chrono::hours{2}),
               wildframe::metadata::MetadataError);

  // Restore permissions so the original file is readable for the
  // invariant check and so TearDown can remove the tree.
  std::filesystem::permissions(raw_in_blocked.parent_path(),
                               std::filesystem::perms::owner_all,
                               std::filesystem::perm_options::replace);

  const auto blocked_after = ReadXmp(blocked_sidecar);
  EXPECT_EQ(
      ReadKey(blocked_before, "Xmp.wildframe_provenance.analysis_timestamp"),
      ReadKey(blocked_after, "Xmp.wildframe_provenance.analysis_timestamp"))
      << "failed write must not alter the target sidecar";

  // The original sidecar (different directory, untouched) is also
  // still valid.
  const auto reread = ReadXmp(sidecar);
  EXPECT_EQ(ReadKey(reread, "Xmp.wildframe_provenance.analysis_timestamp"),
            ReadKey(original, "Xmp.wildframe_provenance.analysis_timestamp"));
}

TEST_F(SidecarWriterTest, LeavesNoTempArtifactsAfterSuccess) {
  const auto raw = RawFixture("IMG_0007.CR3");

  wildframe::metadata::WriteProvenanceSidecar(raw, kFixedInstant);

  bool saw_temp = false;
  for (const auto& entry : std::filesystem::directory_iterator{temp_dir_}) {
    if (entry.path().filename().string().find(".wf-tmp-") !=
        std::string::npos) {
      saw_temp = true;
      break;
    }
  }
  EXPECT_FALSE(saw_temp) << "temp file was not cleaned up";
}

}  // namespace
