#pragma once

#include "mongo/db/pipeline/document_source.h"

namespace mongo {
// A dummy DocumentSource for triggering the eof of an inner group stage
class MkEOF final : public DocumentSource {
public:
  static constexpr StringData kStageName = "$_mkEOF"_sd;
  const char *getSourceName() const final { return kStageName.rawData(); };
  StageConstraints constraints(Pipeline::SplitState pipeState) const final {
    StageConstraints constraints(
        StreamType::kBlocking, PositionRequirement::kNone,
        HostTypeRequirement::kNone, DiskUseRequirement::kWritesTmpData,
        FacetRequirement::kAllowed, TransactionRequirement::kAllowed,
        LookupRequirement::kAllowed, UnionRequirement::kAllowed);
    return constraints;
  }
  GetNextResult doGetNext() final { return GetNextResult::makeEOF(); };
  boost::optional<DistributedPlanLogic> distributedPlanLogic() final {
    return boost::none;
  }
  Value serialize(boost::optional<ExplainOptions::Verbosity> explain =
                      boost::none) const final {
    return Value(Document{{"$_mkEOF", Value(1)}});
  }
  explicit MkEOF(const boost::intrusive_ptr<ExpressionContext> &expCtx)
      : DocumentSource(kStageName, expCtx) {}
};

} // namespace mongo
