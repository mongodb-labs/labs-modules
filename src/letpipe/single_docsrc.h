#pragma once

#include <boost/intrusive_ptr.hpp>
#include <boost/optional.hpp>
#include <map>
#include <memory>
#include <tuple>
#include <vector>

#include "mongo/db/pipeline/document_source.h"
#include "mongo/db/pipeline/expression.h"
#include "mongo/db/pipeline/expression_context.h"
#include "mongo/db/pipeline/expression_visitor.h"
#include "mongo/db/pipeline/lite_parsed_document_source.h"
#include "mongo/db/pipeline/lite_parsed_pipeline.h"
#include "mongo/db/pipeline/pipeline.h"
#include "mongo/db/pipeline/variable_validation.h"
#include "mongo/db/pipeline/variables.h"

namespace mongo {
class SingleDocSrc final : public DocumentSource {
public:
  static constexpr StringData kStageName = "$_singleDocSrc"_sd;

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
    return Value(Document{{"$_singleDocSrc", _doc}});
  }

  SingleDocSrc(const boost::intrusive_ptr<ExpressionContext> &expCtx,
               Document doc)
      : DocumentSource(kStageName, expCtx), _doc(doc), _consumed(false) {}

  DocumentSource::GetNextResult doGetNext() {
    if (_consumed) {
      return DocumentSource::GetNextResult::makeEOF();
    }
    _consumed = true;
    return std::move(_doc);
  }

private:
  Document _doc;
  bool _consumed;
};
} // namespace mongo
