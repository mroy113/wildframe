#include "wildframe/metadata/metadata.hpp"

#include <gtest/gtest.h>

#include <filesystem>

#include "wildframe/metadata/deterministic_metadata.hpp"

namespace {

TEST(ReadExifStub, PopulatesFilesystemFields) {
  const std::filesystem::path raw_path{"/fixtures/IMG_0001.CR3"};
  const auto metadata = wildframe::metadata::ReadExif(raw_path);
  EXPECT_EQ(metadata.file_path, raw_path);
  EXPECT_EQ(metadata.file_name, "IMG_0001.CR3");
}

TEST(ReadExifStub, LeavesExifSourcedFieldsNullopt) {
  const std::filesystem::path raw_path{"/fixtures/IMG_0042.CR3"};
  const auto metadata = wildframe::metadata::ReadExif(raw_path);
  EXPECT_FALSE(metadata.capture_datetime.has_value());
  EXPECT_FALSE(metadata.camera_model.has_value());
  EXPECT_FALSE(metadata.lens_model.has_value());
  EXPECT_FALSE(metadata.focal_length.has_value());
  EXPECT_FALSE(metadata.aperture.has_value());
  EXPECT_FALSE(metadata.shutter_speed.has_value());
  EXPECT_FALSE(metadata.iso.has_value());
  EXPECT_FALSE(metadata.gps_lat.has_value());
  EXPECT_FALSE(metadata.gps_lon.has_value());
  EXPECT_FALSE(metadata.image_dimensions.has_value());
}

TEST(ReadExifStub, IsDeterministicAcrossInputs) {
  // Two different paths should both yield the same sentinel shape —
  // filesystem-derived fields track the path, every EXIF-sourced field
  // stays nullopt. This pins the contract M5-02 will eventually break
  // when real Exiv2 reads land.
  const auto first = wildframe::metadata::ReadExif("/fixtures/a.CR3");
  const auto second = wildframe::metadata::ReadExif("/fixtures/b.CR3");
  EXPECT_EQ(first.capture_datetime.has_value(),
            second.capture_datetime.has_value());
  EXPECT_EQ(first.camera_model.has_value(), second.camera_model.has_value());
  EXPECT_EQ(first.focal_length.has_value(), second.focal_length.has_value());
  EXPECT_EQ(first.iso.has_value(), second.iso.has_value());
}

}  // namespace
