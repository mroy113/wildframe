#include "wildframe/metadata/sidecar_writer.hpp"

#include <array>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <exiv2/error.hpp>
#include <exiv2/properties.hpp>
#include <exiv2/value.hpp>
#include <exiv2/xmp_exiv2.hpp>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iterator>
#include <mutex>
#include <random>
#include <string>
#include <string_view>
#include <system_error>

#include "wildframe/metadata/metadata_error.hpp"
#include "wildframe/metadata/version.hpp"

namespace wildframe::metadata {

namespace {

constexpr std::string_view kProvenanceUri{
    "https://wildframe.app/ns/provenance/1.0/"};
constexpr std::string_view kProvenancePrefix{"wildframe_provenance"};

constexpr std::string_view kDetectorModelName{"stub"};
constexpr std::string_view kDetectorModelVersion{"0.0.0"};
constexpr std::string_view kFocusAlgorithmVersion{"0.0.0"};

// Exiv2's XmpProperties namespace registry is process-global and not
// documented as thread-safe before 0.28. `std::call_once` guards the
// one registration site so the pipeline's worker thread and any GUI-
// side read path (M5-09) share the same registered prefix without
// racing on the registry.
void EnsureNamespaceRegistered() {
  static std::once_flag flag;
  std::call_once(flag, [] {
    Exiv2::XmpProperties::registerNs(std::string{kProvenanceUri},
                                     std::string{kProvenancePrefix});
  });
}

std::string FormatIso8601Utc(std::chrono::system_clock::time_point point) {
  const auto truncated =
      std::chrono::time_point_cast<std::chrono::seconds>(point);
  const std::time_t as_time_t = std::chrono::system_clock::to_time_t(truncated);
  std::tm broken_down{};
  // `gmtime_r` is the POSIX thread-safe variant; `std::gmtime` writes
  // to a shared static buffer that clang-tidy's
  // `concurrency-mt-unsafe` correctly rejects.
  ::gmtime_r(&as_time_t, &broken_down);
  std::array<char, 32> buffer{};
  const std::size_t written = std::strftime(buffer.data(), buffer.size(),
                                            "%Y-%m-%dT%H:%M:%SZ", &broken_down);
  if (written == 0) {
    throw MetadataError{"failed to format analysis_timestamp"};
  }
  return std::string{buffer.data(), written};
}

std::filesystem::path SidecarPathFor(const std::filesystem::path& raw_path) {
  std::filesystem::path result = raw_path;
  result += ".xmp";
  return result;
}

std::filesystem::path TempPathFor(const std::filesystem::path& sidecar) {
  // Same-directory rename is atomic on every filesystem Wildframe
  // targets (APFS, HFS+, ext4). A randomized suffix keeps two
  // concurrent runs writing against different RAWs in the same
  // directory from colliding; the MVP single-worker model makes
  // collision already unlikely, but the suffix costs nothing.
  std::random_device random_device;
  std::uniform_int_distribution<std::uint64_t> distribution;
  std::string suffix = ".wf-tmp-";
  const std::uint64_t token = distribution(random_device);
  constexpr int kHexDigits = 16;
  for (int shift = (kHexDigits - 1) * 4; shift >= 0; shift -= 4) {
    const auto nibble = static_cast<unsigned>((token >> shift) & 0xFU);
    suffix.push_back(
        static_cast<char>(nibble < 10 ? '0' + nibble : 'a' + (nibble - 10)));
  }
  std::filesystem::path result = sidecar;
  result += suffix;
  return result;
}

Exiv2::XmpData LoadExistingSidecar(const std::filesystem::path& sidecar) {
  Exiv2::XmpData existing;
  if (!std::filesystem::exists(sidecar)) {
    return existing;
  }
  std::ifstream stream{sidecar, std::ios::binary};
  if (!stream) {
    throw MetadataError{"failed to open existing sidecar: " + sidecar.string()};
  }
  const std::string packet{std::istreambuf_iterator<char>{stream},
                           std::istreambuf_iterator<char>{}};
  if (Exiv2::XmpParser::decode(existing, packet) != 0) {
    throw MetadataError{"failed to parse existing sidecar: " +
                        sidecar.string()};
  }
  return existing;
}

void DropOwnedProvenance(Exiv2::XmpData& xmp) {
  // Wildframe owns the `wildframe_provenance:*` namespace in full
  // (METADATA.md §1.3) — drop any pre-existing keys in that group
  // before repopulating so a re-analysis under a new pipeline version
  // doesn't leave stale fields behind.
  auto iter = xmp.begin();
  while (iter != xmp.end()) {
    if (iter->groupName() == std::string{kProvenancePrefix}) {
      iter = xmp.erase(iter);
    } else {
      ++iter;
    }
  }
}

void AddXmpText(Exiv2::XmpData& xmp, const std::string& key,
                const std::string& text) {
  // `DropOwnedProvenance` already cleared any pre-existing entries
  // under this key before this call, so `add` is an insert, not a
  // duplicate — the Exiv2-documented read/setValue/add sequence.
  // Prefer `add` over `xmp[key] = text` to avoid
  // `cppcoreguidelines-pro-bounds-avoid-unchecked-container-access`
  // on Exiv2's insert-or-assign `operator[]` (STYLE §2.10 — the real
  // fix, not a suppression).
  Exiv2::XmpTextValue value;
  value.read(text);
  if (xmp.add(Exiv2::XmpKey{key}, &value) != 0) {
    throw MetadataError{"failed to add XMP datum " + key};
  }
}

void SetProvenanceFields(
    Exiv2::XmpData& xmp,
    std::chrono::system_clock::time_point analysis_timestamp) {
  AddXmpText(xmp, "Xmp.wildframe_provenance.analysis_timestamp",
             FormatIso8601Utc(analysis_timestamp));
  AddXmpText(xmp, "Xmp.wildframe_provenance.pipeline_version",
             std::string{kPipelineVersion});
  AddXmpText(xmp, "Xmp.wildframe_provenance.detector_model_name",
             std::string{kDetectorModelName});
  AddXmpText(xmp, "Xmp.wildframe_provenance.detector_model_version",
             std::string{kDetectorModelVersion});
  AddXmpText(xmp, "Xmp.wildframe_provenance.focus_algorithm_version",
             std::string{kFocusAlgorithmVersion});
}

void WritePacketAtomic(const std::filesystem::path& sidecar,
                       const std::string& packet) {
  const auto temp = TempPathFor(sidecar);
  try {
    {
      std::ofstream out{temp, std::ios::binary | std::ios::trunc};
      if (!out) {
        throw MetadataError{"failed to open temp sidecar: " + temp.string()};
      }
      out.write(packet.data(), static_cast<std::streamsize>(packet.size()));
      out.flush();
      if (!out) {
        throw MetadataError{"failed to write temp sidecar: " + temp.string()};
      }
    }
    std::error_code rename_error;
    std::filesystem::rename(temp, sidecar, rename_error);
    if (rename_error) {
      throw MetadataError{"failed to finalize sidecar " + sidecar.string() +
                          ": " + rename_error.message()};
    }
  } catch (...) {
    std::error_code cleanup_error;
    std::filesystem::remove(temp, cleanup_error);
    throw;
  }
}

}  // namespace

void WriteProvenanceSidecar(
    const std::filesystem::path& raw_path,
    std::chrono::system_clock::time_point analysis_timestamp) {
  EnsureNamespaceRegistered();

  const auto sidecar = SidecarPathFor(raw_path);

  Exiv2::XmpData xmp;
  std::string packet;
  try {
    xmp = LoadExistingSidecar(sidecar);
    DropOwnedProvenance(xmp);
    SetProvenanceFields(xmp, analysis_timestamp);
    if (Exiv2::XmpParser::encode(packet, xmp) != 0) {
      throw MetadataError{"failed to encode XMP for " + sidecar.string()};
    }
  } catch (const MetadataError&) {
    throw;
  } catch (const Exiv2::Error& exiv2_error) {
    throw MetadataError{std::string{"Exiv2 error on "} + sidecar.string() +
                        ": " + exiv2_error.what()};
  }

  WritePacketAtomic(sidecar, packet);
}

}  // namespace wildframe::metadata
