#pragma once

#include <atomic>
#include <future>
#include <memory>
#include <optional>
#include <queue>
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
#include "mongo/util/future.h"

namespace mongo {

class DocumentSourceSimpSWindow final : public DocumentSource {
public:
  static constexpr StringData kStageName = "$simpSWindow"_sd;

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
    return Value(Document{
        {"$simpSWindow", Value(Document({{"n", _nWindow}, {"gap", _gap}}))}});
  }
  boost::optional<DistributedPlanLogic> distributedPlanLogic() final {
    return boost::none;
  }

protected:
  GetNextResult doGetNext() final;

private:
  enum SWinState : uint8_t { GET, POP, PARTIAL, WINDOW };

  explicit DocumentSourceSimpSWindow(
      const boost::intrusive_ptr<ExpressionContext> &expCtx, int nWindow,
      int gap)
      : DocumentSource(kStageName, expCtx), _nWindow(nWindow), _gap(gap),
        _nElem(0), _windowed(false), _state(GET) {}

  void startTimer();
  void getNext();

  int _nWindow;
  int _gap;
  int _nElem;
  bool _windowed;
  SWinState _state;
};

} // namespace mongo
