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

class DocumentSourceRandomDelay final : public DocumentSource {
public:
    static constexpr StringData kStageName = "$randomDelay"_sd;

    static boost::intrusive_ptr<DocumentSource> createFromBson(
        BSONElement elem, const boost::intrusive_ptr<ExpressionContext>& expCtx);
    const char* getSourceName() const final {
        return kStageName.rawData();
    }
    StageConstraints constraints(Pipeline::SplitState pipeState) const final {
        StageConstraints constraints(StreamType::kBlocking,
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
        return Value(Document{{"$randomDelay", Value(_max_delay)}});
    }
    boost::optional<DistributedPlanLogic> distributedPlanLogic() final {
        return boost::none;
    }


protected:
    GetNextResult doGetNext() final;

private:
    explicit DocumentSourceRandomDelay(const boost::intrusive_ptr<ExpressionContext>& expCtx,
                                     int max_delay)
        : DocumentSource(kStageName, expCtx), _max_delay(max_delay) {}

    int _max_delay;
};

}  // namespace mongo
