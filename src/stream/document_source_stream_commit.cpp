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

#define MONGO_LOGV2_DEFAULT_COMPONENT ::mongo::logv2::LogComponent::kDefault

#include "mongo/bson/json.h"
#include "document_source_stream_commit.h"
#include "mongo/logv2/log.h"
#include "mongo/platform/basic.h"
#include "mongo/db/pipeline/pipeline.h"

namespace mongo {

DocumentSourceStreamCommit::DocumentSourceStreamCommit(
    const boost::intrusive_ptr<ExpressionContext>& pExpCtx,
    const std::shared_ptr<cppkafka::Consumer> consumer)
    : DocumentSource(kStageName, pExpCtx), _consumer(consumer){}


// Macro to register the document source.
REGISTER_DOCUMENT_SOURCE(streamCommit,
                         LiteParsedDocumentSourceDefault::parse,
                         DocumentSourceStreamCommit::createFromBson,
                         AllowedWithApiStrict::kAlways);

DocumentSource::GetNextResult DocumentSourceStreamCommit::doGetNext() {
    auto nextInput = pSource->getNext();

    auto doc = nextInput.getDocument();

    if (nextInput.isEOF()) {
        return GetNextResult::makeEOF();
    }

    if (nextInput.isUnblock()) {
        auto message = doc.metadata().getStreamMessage();

        _consumer->commit(*message);

        std::string topicName = "json-quickstart";
        int partition = message->get_partition();
        int64_t offset = message->get_offset();


        cppkafka::TopicPartitionList list = {cppkafka::TopicPartition(topicName, message->get_partition())};

        cppkafka::TopicPartitionList _offsetsCommitted = _consumer->get_offsets_committed(list);

        invariant(_offsetsCommitted[0].get_topic() == topicName);
        invariant(_offsetsCommitted[0].get_partition() == partition);
        invariant(_offsetsCommitted[0].get_offset() == offset + 1);

        LOGV2(999999, "Successful commit", "topic"_attr = _resultList[0].get_topic(), "partition"_attr = _resultList[0].get_partition(), "offset"_attr = _resultList[0].get_offset());
    }

    return nextInput;
}

Value DocumentSourceStreamCommit::serialize(boost::optional<ExplainOptions::Verbosity> explain) const {
    MutableDocument insides;

    // TODO.

    return Value{Document{{getSourceName(), insides.freezeToValue()}}};
}


boost::intrusive_ptr<DocumentSourceStreamCommit> DocumentSourceStreamCommit::create(
    const boost::intrusive_ptr<ExpressionContext>& pExpCtx,
    const std::shared_ptr<cppkafka::Consumer> consumer) {
    boost::intrusive_ptr<DocumentSourceStreamCommit> source(new DocumentSourceStreamCommit(
        pExpCtx, consumer));
    return source;
};


boost::intrusive_ptr<DocumentSource> DocumentSourceStreamCommit::createFromBson(
    BSONElement elem, const boost::intrusive_ptr<ExpressionContext>& pExpCtx) {

    MONGO_UNREACHABLE;
}

}  // namespace mongo
