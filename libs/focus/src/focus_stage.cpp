#include "wildframe/focus/focus_stage.hpp"

#include <memory>
#include <optional>
#include <string_view>

#include "wildframe/detect/bbox.hpp"
#include "wildframe/focus/focus.hpp"
#include "wildframe/log/log.hpp"
#include "wildframe/orchestrator/pipeline_stage.hpp"

namespace wildframe::focus {

namespace {

class FocusStage final : public orchestrator::PipelineStage {
 public:
  [[nodiscard]] std::string_view Name() const noexcept override {
    return kName;
  }

  orchestrator::StageResult Process(
      orchestrator::StageContext& context) override {
    FocusResult focus;
    if (context.preview.has_value()) {
      std::optional<detect::BBox> primary_subject;
      if (context.detection.has_value()) {
        primary_subject = context.detection->primary_subject_box;
      }
      focus = Score(*context.preview, primary_subject, FocusConfig{});
    }
    WF_DEBUG(log::focus, "focus ran: keeper_score={}", focus.keeper_score);
    context.focus = focus;
    return {};
  }

 private:
  static constexpr std::string_view kName{"focus"};
};

}  // namespace

std::unique_ptr<orchestrator::PipelineStage> MakeFocusStage() {
  return std::make_unique<FocusStage>();
}

}  // namespace wildframe::focus
