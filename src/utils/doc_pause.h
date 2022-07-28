#pragma once

#include "mongo/db/pipeline/document_source.h"

namespace mongo {
class DocPause final : public DocumentSource {
public:
  static constexpr StringData kStageName = "$_docPause"_sd;

  boost::optional<DistributedPlanLogic> distributedPlanLogic() {
    return boost::none;
  }

  StageConstraints constraints(Pipeline::SplitState pipeState) const override {
    StageConstraints res(StageConstraints::StreamType::kStreaming,
                         StageConstraints::PositionRequirement::kNone,
                         StageConstraints::HostTypeRequirement::kNone,
                         StageConstraints::DiskUseRequirement::kNoDiskUse,
                         StageConstraints::FacetRequirement::kAllowed,
                         StageConstraints::TransactionRequirement::kAllowed,
                         StageConstraints::LookupRequirement::kAllowed,
                         StageConstraints::UnionRequirement::kAllowed);
    res.isIndependentOfAnyCollection = true;
    return res;
  }

  const char *getSourceName() const final { return kStageName.rawData(); }

  Value serialize(boost::optional<explain::VerbosityEnum> explain) const {
    return Value(Document{{"$_docPause", _doc}});
  }

  DocPause(const boost::intrusive_ptr<ExpressionContext> &expCtx,
           const Document doc)
      : DocumentSource(kStageName, expCtx), _doc(doc), _consumed(false) {}

  DocumentSource::GetNextResult doGetNext() {
    if (_consumed) {
      return DocumentSource::GetNextResult::makePauseExecution();
    }
    _consumed = true;
    return std::move(_doc);
  }

private:
  Document _doc;
  bool _consumed;
};

} // namespace mongo
