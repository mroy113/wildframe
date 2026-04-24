#include "wildframe/focus/focus.hpp"

#include <optional>

#include "wildframe/detect/bbox.hpp"
#include "wildframe/raw/preview_image.hpp"

namespace wildframe::focus {

FocusResult Score(const raw::PreviewImage& /*preview*/,
                  const std::optional<detect::BBox>& /*primary_subject*/,
                  const FocusConfig& /*config*/) {
  // M4-06 will run the real OpenCV-backed keeper-score computation
  // here. The stub returns the "no quality signal" sentinel
  // documented on `FocusResult`; relying on default-member-
  // initializers keeps the field list single-sourced in the header
  // (`docs/STYLE.md` §2.11) — consumers see the identical shape when
  // the stub is replaced.
  return FocusResult{};
}

}  // namespace wildframe::focus
