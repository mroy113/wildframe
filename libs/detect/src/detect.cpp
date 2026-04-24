#include "wildframe/detect/detect.hpp"

#include "wildframe/raw/preview_image.hpp"

namespace wildframe::detect {

DetectionResult Detect(const raw::PreviewImage& /*preview*/,
                       const DetectConfig& /*config*/) {
  // M3-05 will run the real ONNX Runtime session here. The stub
  // returns the "no detection" sentinel documented on
  // `DetectionResult`; relying on default-member-initializers keeps
  // the field list single-sourced in the header (`docs/STYLE.md`
  // §2.11) — consumers see the identical shape when the stub is
  // replaced.
  return DetectionResult{};
}

}  // namespace wildframe::detect
