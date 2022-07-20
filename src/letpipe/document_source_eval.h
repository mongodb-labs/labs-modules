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

#include "single_docsrc.h"

namespace mongo {

class DocumentSourceEval final : public DocumentSource {
public:
  static constexpr StringData kStageName = "$eval"_sd;

  static boost::intrusive_ptr<DocumentSource>
  createFromBson(BSONElement elem,
                 const boost::intrusive_ptr<ExpressionContext> &pExpCtx);

  boost::optional<DistributedPlanLogic> distributedPlanLogic() {
    return boost::none;
  }
  StageConstraints constraints(Pipeline::SplitState pipeState) const override {
    return {StageConstraints::StreamType::kStreaming,
            StageConstraints::PositionRequirement::kNone,
            StageConstraints::HostTypeRequirement::kNone,
            StageConstraints::DiskUseRequirement::kNoDiskUse,
            StageConstraints::FacetRequirement::kAllowed,
            StageConstraints::TransactionRequirement::kAllowed,
            StageConstraints::LookupRequirement::kAllowed,
            StageConstraints::UnionRequirement::kAllowed};
  }

  const char *getSourceName() const final { return kStageName.rawData(); }

  Value serialize(boost::optional<explain::VerbosityEnum> explain) const {
    return Value(Document{{"$eval", _expr->serialize(false)}});
  }

private:
  explicit DocumentSourceEval(
      const boost::intrusive_ptr<ExpressionContext> &expCtx,
      boost::intrusive_ptr<Expression> expr)
      : DocumentSource(kStageName, expCtx), _expr(std::move(expr)) {}

  DocumentSource::GetNextResult doGetNext();
  std::unique_ptr<Pipeline, PipelineDeleter>
  expandPipeline(const Document &doc);

  boost::intrusive_ptr<Expression> _expr;
};

} // namespace mongo
