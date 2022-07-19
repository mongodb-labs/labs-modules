#define MONGO_LOGV2_DEFAULT_COMPONENT ::mongo::logv2::LogComponent::kDefault

#include "mongo/logv2/log.h"

#include "mongo/platform/basic.h"

#include "document_source_letpipe.h"

#include <memory>
#include <vector>

#include "mongo/base/string_data.h"
#include "mongo/bson/bsonobj.h"
#include "mongo/bson/bsonobjbuilder.h"
#include "mongo/bson/bsontypes.h"
#include "mongo/db/exec/document_value/document.h"
#include "mongo/db/exec/document_value/value.h"
#include "mongo/db/pipeline/document_source_tee_consumer.h"
#include "mongo/db/pipeline/expression_context.h"
#include "mongo/db/pipeline/field_path.h"
#include "mongo/db/pipeline/pipeline.h"
#include "mongo/db/pipeline/tee_buffer.h"
#include "mongo/util/assert_util.h"
#include "mongo/util/str.h"

namespace mongo {

REGISTER_DOCUMENT_SOURCE(letp, DocumentSourceLetp::LiteParsed::parse,
                         DocumentSourceLetp::createFromBson,
                         AllowedWithApiStrict::kAlways);

std::unique_ptr<DocumentSourceLetp::LiteParsed>
DocumentSourceLetp::LiteParsed::parse(const NamespaceString &nss,
                                      const BSONElement &spec) {
  auto specObj = spec.Obj();
  auto pipeline = specObj["pipeline"];
  auto fromNss = nss;
  boost::optional<LiteParsedPipeline> liteParsedPipeline =
      LiteParsedPipeline(fromNss, parsePipelineFromBSON(pipeline));
  return std::make_unique<DocumentSourceLetp::LiteParsed>(
      spec.fieldName(), std::move(fromNss), std::move(liteParsedPipeline));
}

boost::intrusive_ptr<DocumentSource> DocumentSourceLetp::createFromBson(
    BSONElement elem, const boost::intrusive_ptr<ExpressionContext> &pExpCtx) {

  /*
Syntax:
{$letp:
  {
    vars: { },
    pipeline: [],
  }
}
     */
  uassert(99999, "$letp only support object as its argument",
          elem.type() == Object);

  BSONObj vars;
  std::vector<BSONObj> pipeline;

  for (auto &&arg : elem.Obj()) {
    const auto name = arg.fieldNameStringData();
    if (name == "vars"_sd) {
      vars = arg.Obj();
    } else if (name == "pipeline"_sd) {
      pipeline = parsePipelineFromBSON(arg);
    } else {
      uasserted(99999, str::stream()
                           << "Unknown field for $letp: " << arg.fieldName());
    }
  }

  boost::intrusive_ptr<DocumentSourceLetp> res(
      new DocumentSourceLetp(pExpCtx, std::move(pipeline), std::move(vars)));
  return res;
}

DocumentSourceLetp::DocumentSourceLetp(
    const boost::intrusive_ptr<ExpressionContext> &expCtx,
    std::vector<BSONObj> pipeline, BSONObj vars)
    : DocumentSource(kStageName, expCtx), _vars(expCtx->variables),
      _vps(expCtx->variablesParseState.copyWith(_vars.useIdGenerator())),
      _letBinds{} {
  auto ns = expCtx->ns;
  const auto &resolvedNs = expCtx->getResolvedNamespace(ns);
  _resolvedNs = resolvedNs.ns;
  _resolvedPipeline = resolvedNs.pipeline;
  _resolvedPipeline.insert(_resolvedPipeline.end(), pipeline.begin(),
                           pipeline.end());
  _innerExpCtx = expCtx->copyForSubPipeline(resolvedNs.ns, resolvedNs.uuid);

  for (auto &&var : vars) {
    const auto varName = var.fieldNameStringData();
    variableValidation::validateNameForUserWrite(varName);
    _letBinds.emplace_back(varName.toString(),
                           Expression::parseOperand(
                               expCtx.get(), var, expCtx->variablesParseState),
                           _vps.defineVariable(varName));
  }
}

void DocumentSourceLetp::resolveLetVars(const Document &doc, Variables *vars) {
  LOGV2(99999999, "resolving let vars with: ", "doc"_attr = doc);
  for (auto &bind : _letBinds) {
    auto val = bind.expression->evaluate(doc, &pExpCtx->variables);
    vars->setConstantValue(bind.id, val);
  }
}

std::unique_ptr<Pipeline, PipelineDeleter>
DocumentSourceLetp::buildPipeline(const Document &doc) {
  _vars.copyToExpCtx(_vps, _innerExpCtx.get());
  resolveLetVars(doc, &_innerExpCtx->variables);

  LOGV2(99999999, "resolved pipeline: ", "pipe"_attr = _resolvedPipeline);
  MakePipelineOptions mkPipeOpt;
  mkPipeOpt.optimize = false;
  mkPipeOpt.attachCursorSource = false;
  auto pipe =
      Pipeline::makePipeline(_resolvedPipeline, _innerExpCtx, mkPipeOpt);
  pipe->addInitialSource(new SingleDocSrc(pExpCtx, doc));
  return pipe;
}

DocumentSource::GetNextResult DocumentSourceLetp::doGetNext() {
  auto next = pSource->getNext();
  if (!next.isAdvanced()) {
    return next;
  }
  auto doc = next.releaseDocument();
  LOGV2(999999, "get next", "next"_attr = doc);
  auto pipeline = buildPipeline(doc);
  auto ser_pipe = pipeline->serializeToBson();
  for (auto &&p : ser_pipe) {
    LOGV2(99999999, "pipe stage: ", "part"_attr = p);
  }
  auto res = pipeline->getNext();
  LOGV2(999999, "pipeline next: ", "res"_attr = res);
  return std::move(*res);
}

} // namespace mongo
