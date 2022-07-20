#define MONGO_LOGV2_DEFAULT_COMPONENT ::mongo::logv2::LogComponent::kDefault

#include "mongo/logv2/log.h"

#include "mongo/platform/basic.h"

#include "document_source_eval.h"

namespace mongo {

REGISTER_DOCUMENT_SOURCE(eval, LiteParsedDocumentSourceDefault::parse,
                         DocumentSourceEval::createFromBson,
                         AllowedWithApiStrict::kAlways);

boost::intrusive_ptr<DocumentSource> DocumentSourceEval::createFromBson(
    BSONElement elem, const boost::intrusive_ptr<ExpressionContext> &expCtx) {
  LOGV2(99999999, "create from bson", "elem"_attr = elem);
  if (elem.type() == BSONType::Object) {
    auto expr = elem.Obj();
    LOGV2(99999999, "expr extracted", "expr"_attr = expr);
    boost::intrusive_ptr<DocumentSource> res(new DocumentSourceEval(
        expCtx, Expression::parseObject(expCtx.get(), expr,
                                        expCtx->variablesParseState)));
    return res;
  } else if (elem.type() == BSONType::String) {
    auto var = elem.valueStringDataSafe();
    boost::intrusive_ptr<Expression> expr(
        new ExpressionConstant(expCtx.get(), Value(var)));
    LOGV2(99999999, "expr extracted : ", "expr"_attr = expr->serialize(false));
    boost::intrusive_ptr<DocumentSource> res(
        new DocumentSourceEval(expCtx, expr));
    return res;
  }
  uasserted(ErrorCodes::BadValue, "failed to parse $eval");
  MONGO_UNREACHABLE;
}

std::unique_ptr<Pipeline, PipelineDeleter>
DocumentSourceEval::expandPipeline(const Document &doc) {
  auto pipeSpec = _expr->evaluate(doc, &pExpCtx->variables);
  LOGV2(99999999, "evaluated pipeline: ", "pipe"_attr = pipeSpec);
  uassert(99999, "pipeline spec in $eval must evaluates into an array",
          pipeSpec.isArray());
  std::vector<BSONObj> pipeParts;
  for (auto &p : pipeSpec.getArray()) {
    uassert(99999, "pipeline stage must be document", p.isObject());
    pipeParts.push_back(p.getDocument().toBson());
  }
  MakePipelineOptions mkPipeOpt;
  mkPipeOpt.optimize = false;
  mkPipeOpt.attachCursorSource = false;
  auto pipe = Pipeline::makePipeline(pipeParts, pExpCtx, mkPipeOpt);
  pipe->addInitialSource(new SingleDocSrc(pExpCtx, doc));
  return pipe;
}

DocumentSource::GetNextResult DocumentSourceEval::doGetNext() {
  auto next = pSource->getNext();
  if (!next.isAdvanced()) {
    return next;
  }
  auto doc = next.releaseDocument();
  auto pipeline = expandPipeline(doc);
  auto ser_pipe = pipeline->serializeToBson();
  for (auto &&p : ser_pipe) {
    LOGV2(99999999, "pipe stage: ", "part"_attr = p);
  }
  auto res = pipeline->getNext();
  LOGV2(999999, "pipeline next: ", "res"_attr = res);
  return std::move(*res);
}

} // namespace mongo
