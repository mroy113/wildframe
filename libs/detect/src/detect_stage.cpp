#include "wildframe/detect/detect_stage.hpp"

#include <memory>
#include <string_view>
#include <utility>

#include "wildframe/detect/detect.hpp"
#include "wildframe/log/log.hpp"
#include "wildframe/orchestrator/pipeline_stage.hpp"

namespace wildframe::detect {

namespace {

class DetectStage final : public orchestrator::PipelineStage {
 public:
  [[nodiscard]] std::string_view Name() const noexcept override {
    return kName;
  }

  orchestrator::StageResult Process(
      orchestrator::StageContext& context) override {
    // `RawStage` populates `context.preview` before this stage runs
    // (see `docs/ARCHITECTURE.md` §3.2); the explicit `has_value()`
    // check exists to satisfy
    // `bugprone-unchecked-optional-access`, whose dataflow cannot
    // model the stage-ordering invariant. Default-constructing
    // `DetectionResult` already produces the stub's sentinel, so
    // the impossible missing-preview case still yields a
    // well-formed result.
    DetectionResult detection;
    if (context.preview.has_value()) {
      detection = Detect(*context.preview, DetectConfig{});
    }
    WF_DEBUG(log::detect, "detect ran: bird_present={} bird_count={}",
             detection.bird_present, detection.bird_count);
    context.detection = std::move(detection);
    return {};
  }

 private:
  static constexpr std::string_view kName{"detect"};
};

}  // namespace

std::unique_ptr<orchestrator::PipelineStage> MakeDetectStage() {
  return std::make_unique<DetectStage>();
}

}  // namespace wildframe::detect
