#include "wildframe/metadata/metadata.hpp"

#include <filesystem>

#include "wildframe/metadata/deterministic_metadata.hpp"

namespace wildframe::metadata {

DeterministicMetadata ReadExif(const std::filesystem::path& raw_path) {
  // M5-02 will run the real Exiv2-backed read here. The stub returns
  // the filesystem-derived fields only — every EXIF-sourced field stays
  // `std::nullopt` per `DeterministicMetadata`'s "missing tag stays
  // nullopt" contract. Relying on default-member-initializers keeps
  // the field list single-sourced in the header (`docs/STYLE.md` §2.11)
  // so consumers see the identical shape when the stub is replaced.
  DeterministicMetadata metadata;
  metadata.file_path = raw_path;
  metadata.file_name = raw_path.filename().string();
  return metadata;
}

}  // namespace wildframe::metadata
