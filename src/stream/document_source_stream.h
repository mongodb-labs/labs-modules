/**
 *    Copyright (C) 2018-present MongoDB, Inc.
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the Server Side Public License, version 1,
 *    as published by MongoDB, Inc.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    Server Side Public License for more details.
 *
 *    You should have received a copy of the Server Side Public License
 *    along with this program. If not, see
 *    <http://www.mongodb.com/licensing/server-side-public-license>.
 *
 *    As a special exception, the copyright holders give permission to link the
 *    code of portions of this program with the OpenSSL library under certain
 *    conditions as described in each individual source file and distribute
 *    linked combinations including the program with the OpenSSL library. You
 *    must comply with the Server Side Public License in all respects for
 *    all of the code used other than as permitted herein. If you modify file(s)
 *    with this exception, you may extend this exception to your version of the
 *    file(s), but you are not obligated to do so. If you do not wish to do so,
 *    delete this exception statement from your version. If you delete this
 *    exception statement from all source files in the program, then also delete
 *    it in the license file.
 */

#pragma once

#include <cppkafka/cppkafka.h>

#include "mongo/db/pipeline/document_source.h"
#include "mongo/db/pipeline/pipeline.h"

#include "stream_controller.h"

namespace mongo {

class DocumentSourceStream final : public DocumentSource {
public:
    static constexpr StringData kStageName = "$stream"_sd;

    // Create a new $stream stage.
    static boost::intrusive_ptr<DocumentSourceStream> create(
        const boost::intrusive_ptr<ExpressionContext>& pExpCtx,
        std::unique_ptr<Pipeline, PipelineDeleter> pipeline,
        const BSONObj metadata,
        const cppkafka::Configuration kafkaConfig,
        const std::string kafkaTopic,
        const std::string kafkaTopicFormat);


    static boost::intrusive_ptr<DocumentSource> createFromBson(
        BSONElement elem, const boost::intrusive_ptr<ExpressionContext>& pExpCtx);

    StageConstraints constraints(Pipeline::SplitState pipeState) const override {
        return {StreamType::kStreaming,
                PositionRequirement::kNone,
                HostTypeRequirement::kNone,
                DiskUseRequirement::kNoDiskUse,
                FacetRequirement::kAllowed,
                TransactionRequirement::kAllowed,
                LookupRequirement::kAllowed,
                UnionRequirement::kAllowed,
                ChangeStreamRequirement::kAllowlist};
    }

    const char* getSourceName() const final {
        return kStageName.rawData();
    }

    Value serialize(boost::optional<ExplainOptions::Verbosity> explain = boost::none) const final;

    boost::optional<DistributedPlanLogic> distributedPlanLogic() final {
        return boost::none;
    }

private:
    DocumentSourceStream(
        const boost::intrusive_ptr<ExpressionContext>& pExpCtx,
        std::unique_ptr<Pipeline, PipelineDeleter> pipeline,
        const BSONObj metadata,
        const cppkafka::Configuration kafkaConfig,
        const std::string kafkaTopic,
        const std::string kafkaTopicFormat
    );

    std::unique_ptr<Pipeline, PipelineDeleter> _pipeline;

    boost::intrusive_ptr<StreamController> _streamController;

    std::shared_ptr<cppkafka::Consumer> _consumer;

    GetNextResult doGetNext() final;
};

}  // namespace mongo
