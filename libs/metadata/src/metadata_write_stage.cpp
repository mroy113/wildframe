#include "wildframe/metadata/metadata_write_stage.hpp"

#include <chrono>
#include <memory>
#include <string_view>

#include "wildframe/log/log.hpp"
#include "wildframe/metadata/sidecar_writer.hpp"
#include "wildframe/orchestrator/pipeline_stage.hpp"

namespace wildframe::metadata {

namespace {

class MetadataWriteStage final : public orchestrator::PipelineStage {
 public:
  explicit MetadataWriteStage(
      std::chrono::system_clock::time_point analysis_timestamp) noexcept
      : analysis_timestamp_{analysis_timestamp} {}

  [[nodiscard]] std::string_view Name() const noexcept override {
    return kName;
  }

  orchestrator::StageResult Process(
      orchestrator::StageContext& context) override {
    WriteProvenanceSidecar(context.job.path, analysis_timestamp_);
    WF_DEBUG(log::metadata, "wrote provenance sidecar for {}",
             context.job.path.string());
    return {};
  }

 private:
  static constexpr std::string_view kName{"metadata_write"};
  std::chrono::system_clock::time_point analysis_timestamp_;
};

}  // namespace

std::unique_ptr<orchestrator::PipelineStage> MakeMetadataWriteStage(
    std::chrono::system_clock::time_point analysis_timestamp) {
  return std::make_unique<MetadataWriteStage>(analysis_timestamp);
}

}  // namespace wildframe::metadata
