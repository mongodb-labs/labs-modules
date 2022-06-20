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
#include "mongo/util/periodic_runner.h"
#include "mongo/util/periodic_runner_factory.h"

namespace mongo {

class DocumentSourceSimpTWindow final : public DocumentSource {
public:
    static constexpr StringData kStageName = "$simpTWindow"_sd;

    static boost::intrusive_ptr<DocumentSource> createFromBson(
        BSONElement elem, const boost::intrusive_ptr<ExpressionContext>& expCtx);
    const char* getSourceName() const final {
        return kStageName.rawData();
    }
    StageConstraints constraints(Pipeline::SplitState pipeState) const final {
        StageConstraints constraints(StreamType::kStreaming,
                                     PositionRequirement::kNone,
                                     HostTypeRequirement::kNone,
                                     DiskUseRequirement::kWritesTmpData,
                                     FacetRequirement::kAllowed,
                                     TransactionRequirement::kAllowed,
                                     LookupRequirement::kAllowed,
                                     UnionRequirement::kAllowed);
        return constraints;
    }
    Value serialize(boost::optional<ExplainOptions::Verbosity> explain = boost::none) const final {
        return Value(Document{{"$simpTWindow", Value(_duration)}});
    }
    boost::optional<DistributedPlanLogic> distributedPlanLogic() final {
        return boost::none;
    }


protected:
    GetNextResult doGetNext() final;

private:
    explicit DocumentSourceSimpTWindow(const boost::intrusive_ptr<ExpressionContext>& expCtx,
                                       int duration,
                                       std::unique_ptr<PeriodicRunner> runner)
        : DocumentSource(kStageName, expCtx),
          _duration(duration),
          _timerRunner(std::move(runner)),
          _inited(false),
          _windowed(false) {}

    void startTimer();
    void getNext();

    int _duration;
    std::unique_ptr<PeriodicRunner> _timerRunner;
    PeriodicJobAnchor _timer;
    bool _inited;
    std::future<DocumentSource::GetNextResult> _next;
    std::atomic_bool _windowed;
};

}  // namespace mongo
