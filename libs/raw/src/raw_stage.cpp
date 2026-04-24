#include "wildframe/raw/raw_stage.hpp"

#include <memory>
#include <string_view>
#include <utility>

#include "wildframe/log/log.hpp"
#include "wildframe/orchestrator/pipeline_stage.hpp"
#include "wildframe/raw/preview_image.hpp"
#include "wildframe/raw/raw.hpp"

namespace wildframe::raw {

namespace {

class RawStage final : public orchestrator::PipelineStage {
 public:
  [[nodiscard]] std::string_view Name() const noexcept override {
    return kName;
  }

  orchestrator::StageResult Process(
      orchestrator::StageContext& context) override {
    PreviewImage preview = ExtractPreview(context.job.path);
    WF_DEBUG(log::raw, "extracted preview: {}x{} bytes={}", preview.width,
             preview.height, preview.rgb_bytes.size());
    context.preview = std::move(preview);
    return {};
  }

 private:
  static constexpr std::string_view kName{"raw"};
};

}  // namespace

std::unique_ptr<orchestrator::PipelineStage> MakeRawStage() {
  return std::make_unique<RawStage>();
}

}  // namespace wildframe::raw
