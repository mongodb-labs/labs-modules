#pragma once

#include <memory>
#include <utility>

#include "mongo/db/pipeline/accumulation_statement.h"
#include "mongo/db/pipeline/accumulator.h"
#include "mongo/db/pipeline/document_source.h"
#include "mongo/db/pipeline/lite_parsed_document_source.h"
#include "mongo/db/pipeline/lite_parsed_pipeline.h"
#include "mongo/db/pipeline/memory_usage_tracker.h"
#include "mongo/db/pipeline/pipeline.h"
#include "mongo/db/pipeline/transformer_interface.h"
#include "mongo/db/sorter/sorter.h"

namespace mongo {

class DocumentSourceOb final : public DocumentSource {
public:
  static constexpr StringData kStageName = "$ob"_sd;

  static boost::intrusive_ptr<DocumentSource>
  createFromBson(BSONElement elem,
                 const boost::intrusive_ptr<ExpressionContext> &expCtx);
  const char *getSourceName() const final { return kStageName.rawData(); }
  StageConstraints constraints(Pipeline::SplitState pipeState) const final {
    StageConstraints constraints(
        StreamType::kStreaming, PositionRequirement::kNone,
        HostTypeRequirement::kNone, DiskUseRequirement::kWritesTmpData,
        FacetRequirement::kAllowed, TransactionRequirement::kAllowed,
        LookupRequirement::kAllowed, UnionRequirement::kAllowed);
    return constraints;
  }
  Value serialize(boost::optional<ExplainOptions::Verbosity> explain =
                      boost::none) const final {
    return Value(Document{{"$ob", Value(1)}});
  }
  boost::optional<DistributedPlanLogic> distributedPlanLogic() final {
    return boost::none;
  }

protected:
  GetNextResult doGetNext() final;

private:
  explicit DocumentSourceOb(
      const boost::intrusive_ptr<ExpressionContext> &expCtx)
      : DocumentSource(kStageName, expCtx) {}
};

} // namespace mongo
