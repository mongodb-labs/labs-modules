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

class DocumentSourceLetp final : public DocumentSource {

public:
  using pipe_t = std::unique_ptr<Pipeline, PipelineDeleter>;
  using envpair_t = std::tuple<std::string, boost::intrusive_ptr<Expression> &>;
  using env_t = std::map<Variables::Id, envpair_t>;

  static constexpr StringData kStageName = "$letp"_sd;

  static boost::intrusive_ptr<DocumentSource>
  createFromBson(BSONElement elem,
                 const boost::intrusive_ptr<ExpressionContext> &pExpCtx);

  boost::optional<DistributedPlanLogic> distributedPlanLogic() {
    return boost::none;
  }

  // static boost::intrusive_ptr<DocumentSource> create(BSONElement vars,
  //                                                    pipe_t pipe);

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
    return Value(Document{{"$letp", Value(1)}});
  }

  class LiteParsed final : public LiteParsedDocumentSourceNestedPipelines {
  public:
    static std::unique_ptr<LiteParsed> parse(const NamespaceString &nss,
                                             const BSONElement &spec);

    LiteParsed(std::string parseTimeName, NamespaceString foreignNss,
               boost::optional<LiteParsedPipeline> pipeline)
        : LiteParsedDocumentSourceNestedPipelines(std::move(parseTimeName),
                                                  std::move(foreignNss),
                                                  std::move(pipeline)) {}

    PrivilegeVector
    requiredPrivileges(bool isMongos,
                       bool bypassDocumentValidation) const override final {
      PrivilegeVector res;
      Privilege::addPrivilegesToPrivilegeVector(
          &res, std::move(_pipelines[0].requiredPrivileges(
                    isMongos, bypassDocumentValidation)));
      return res;
    }
  };

  struct LetBind {
    LetBind(std::string name, boost::intrusive_ptr<Expression> expression,
            Variables::Id id)
        : name(std::move(name)), expression(std::move(expression)), id(id) {}

    std::string name;
    boost::intrusive_ptr<Expression> expression;
    Variables::Id id;
  };

protected:
private:
  explicit DocumentSourceLetp(
      const boost::intrusive_ptr<ExpressionContext> &pExpCtx,
      std::vector<BSONObj> pipeline, BSONObj vars);

  DocumentSource::GetNextResult doGetNext();
  std::unique_ptr<Pipeline, PipelineDeleter> buildPipeline(const Document &doc);
  void resolveLetVars(const Document &doc, Variables *vars);

  pipe_t _pipe;
  Variables _vars;
  VariablesParseState _vps;
  std::vector<LetBind> _letBinds;
  NamespaceString _resolvedNs;
  std::vector<BSONObj> _resolvedPipeline;

  boost::intrusive_ptr<ExpressionContext> _innerExpCtx;
};

} // namespace mongo
