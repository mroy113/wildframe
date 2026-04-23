#pragma once

/// \file
/// `DeterministicMetadata` — camera/shot facts read out of a RAW file's
/// EXIF block by `wildframe_metadata::ReadExif` (M5-02). "Deterministic"
/// per handoff §13: values originating from the camera at capture time,
/// not from Wildframe's pipeline, so they round-trip identically across
/// re-analyses.
///
/// EXIF-sourced fields are `std::optional<T>`: a missing tag stays
/// `std::nullopt` rather than being coerced to a sentinel, so callers can
/// distinguish "camera did not record this" from "camera recorded zero"
/// (handoff §13, `docs/METADATA.md` §6, §7.4). Filesystem-derived fields
/// (`file_path`, `file_name`) are bare types — `ReadExif` is called with
/// a path, so it cannot produce a record without one.
///
/// Value type, Rule of Zero (`docs/STYLE.md` §2.5). Plain aggregate —
/// public data members use bare `snake_case` per STYLE §2.1.
///
/// MVP does not re-serialize these values into a `wildframe:*` XMP
/// namespace: the standard `exif:` / `tiff:` namespaces already carry
/// them in any downstream tool that reads XMP (`docs/METADATA.md` §6).

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>

namespace wildframe::metadata {

/// Pixel dimensions of the full-resolution image recorded in the RAW
/// (`tiff:ImageWidth` / `tiff:ImageLength`). Width and height are
/// populated together or not at all — a dimension pair with one side
/// unknown is not a valid image size, so the whole struct is wrapped in
/// a `std::optional` on `DeterministicMetadata` rather than making each
/// axis independently optional.
struct ImageDimensions {
  std::uint32_t width = 0;
  std::uint32_t height = 0;
};

/// Camera-recorded metadata for a single RAW file. Populated by the
/// Exiv2-backed reader in M5-02; TB-06's stub reader returns an instance
/// with only the filesystem-derived fields set and every EXIF-sourced
/// field `std::nullopt`. Downstream stages consume this by value or
/// `const&`.
struct DeterministicMetadata {
  /// Absolute canonical path to the RAW file on disk. Filesystem-
  /// derived — `ReadExif` is always called with a path, so this is
  /// always populated and is not wrapped in `std::optional`.
  std::filesystem::path file_path;

  /// File basename including the RAW extension (e.g. `IMG_0421.CR3`).
  /// Filesystem-derived alongside `file_path`; always populated.
  std::string file_name;

  /// Capture timestamp (`exif:DateTimeOriginal`), normalized to UTC by
  /// the reader. Stored as a `time_point` rather than a raw EXIF string
  /// so downstream code can compare and sort without reparsing.
  std::optional<std::chrono::system_clock::time_point> capture_datetime;

  /// Camera body model (`tiff:Model`, e.g. `"Canon EOS R5"`).
  std::optional<std::string> camera_model;

  /// Lens model (`exifEX:LensModel` or `aux:Lens`, e.g.
  /// `"RF 600mm F4 L IS USM"`).
  std::optional<std::string> lens_model;

  /// Focal length in millimeters (`exif:FocalLength`).
  std::optional<float> focal_length;

  /// Aperture as an f-number (`exif:FNumber`). Example: `5.6F` for
  /// f/5.6. The EXIF rational is resolved to a single scalar here.
  std::optional<float> aperture;

  /// Exposure time in seconds (`exif:ExposureTime`). Example: `0.001F`
  /// for 1/1000s. The EXIF rational is resolved to a single scalar here.
  std::optional<float> shutter_speed;

  /// ISO sensitivity (`exif:ISOSpeedRatings`). Unsigned because the
  /// EXIF tag is non-negative by definition; `std::uint32_t` accommodates
  /// the extended-ISO range modern bodies emit beyond the legacy
  /// 16-bit SHORT field.
  std::optional<std::uint32_t> iso;

  /// GPS latitude in decimal degrees (`exif:GPSLatitude`), positive
  /// north / negative south. `double` preserves the sub-meter precision
  /// handheld GNSS receivers emit (`float`'s ~7-digit mantissa would
  /// lose it at the third decimal).
  std::optional<double> gps_lat;

  /// GPS longitude in decimal degrees (`exif:GPSLongitude`), positive
  /// east / negative west. See `gps_lat` for the width rationale.
  std::optional<double> gps_lon;

  /// Full-resolution image dimensions. See `ImageDimensions` above.
  std::optional<ImageDimensions> image_dimensions;
};

}  // namespace wildframe::metadata
