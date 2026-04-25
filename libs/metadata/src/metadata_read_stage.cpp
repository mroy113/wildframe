#include "wildframe/metadata/metadata_read_stage.hpp"

#include <memory>
#include <string_view>
#include <utility>

#include "wildframe/log/log.hpp"
#include "wildframe/metadata/deterministic_metadata.hpp"
#include "wildframe/metadata/metadata.hpp"
#include "wildframe/orchestrator/pipeline_stage.hpp"

namespace wildframe::metadata {

namespace {

class MetadataReadStage final : public orchestrator::PipelineStage {
 public:
  [[nodiscard]] std::string_view Name() const noexcept override {
    return kName;
  }

  orchestrator::StageResult Process(
      orchestrator::StageContext& context) override {
    DeterministicMetadata exif = ReadExif(context.job.path);
    WF_DEBUG(log::metadata, "read_exif ran: file_name={}", exif.file_name);
    context.exif = std::move(exif);
    return {};
  }

 private:
  static constexpr std::string_view kName{"metadata_read"};
};

}  // namespace

std::unique_ptr<orchestrator::PipelineStage> MakeMetadataReadStage() {
  return std::make_unique<MetadataReadStage>();
}

}  // namespace wildframe::metadata
