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

#include <boost/algorithm/string.hpp>
#include <boost/intrusive_ptr.hpp>

#include "document_source_in.h"

#include "mongo/bson/json.h"
#include "mongo/logv2/log.h"
#include "mongo/platform/basic.h"
#include "mongo/db/pipeline/pipeline.h"
#include "mongo/bson/json.h"

namespace mongo {

DocumentSourceIn::DocumentSourceIn(
    const boost::intrusive_ptr<ExpressionContext>& pExpCtx,
    const std::shared_ptr<cppkafka::Consumer> consumer,
    const std::string topic,
    const std::string format)
    : DocumentSource(kStageName, pExpCtx),
    _consumer(consumer),
    _format(format){
        // Subscribe to the topic
        _consumer->subscribe({ topic });
    }


// Macro to register the document source.
REGISTER_DOCUMENT_SOURCE(in,
                         LiteParsedDocumentSourceDefault::parse,
                         DocumentSourceIn::createFromBson,
                         AllowedWithApiStrict::kAlways);

DocumentSource::GetNextResult DocumentSourceIn::doGetNext() {
    cppkafka::Message msg = _consumer->poll();

    // If we managed to get a message
    if (msg) {
        if (msg.is_eof()) {
            // EOF means we've reached the end of the log
            LOGV2(999999, "Received Kafka EOF");
            return GetNextResult::makeEOF();
        }

        if (msg.get_error() && !msg.is_eof()) {
            std::string errorMessage = std::string(msg.get_payload());
            LOGV2(999999, "Kafka Consumer Error", "error"_attr = errorMessage);
        } else {
            std::string payload = std::string(msg.get_payload());

            int64_t offset = msg.get_offset();

            LOGV2(999999, "Received Kafka payload", "payload"_attr = payload, "offset"_attr=offset);

            // Convert string to BSONObj

            BSONObj obj;

            try {
                obj = fromjson(payload);
            } catch (std::exception&) {
                // Message could not be parsed to JSON.
                return GetNextResult::makePauseExecution();
            }

            // Create new BSON with response
            BSONObjBuilder builder = BSONObjBuilder();
            builder.appendElements(obj);

            BSONObj newObj = builder.done();

             // The output doc is the same as the input doc, with the added fields.
            MutableDocument output;

            output.newStorageWithBson(newObj, false);

            // TODO: maybe a cheaper way to transfer ownership to MutableDoc
            output.makeOwned();

            // Update metadata
            output.metadata().setStreamMessage(std::move(msg));
            Document outputDoc = output.freeze();

            // Create new Document from newly constructed BSONObj and return
            return DocumentSource::GetNextResult(std::move(outputDoc));
        }
    }

    return GetNextResult::makePauseExecution();
}

Value DocumentSourceIn::serialize(boost::optional<ExplainOptions::Verbosity> explain) const {
    MutableDocument insides;

    // TODO.

    return Value{Document{{getSourceName(), insides.freezeToValue()}}};
}


boost::intrusive_ptr<DocumentSourceIn> DocumentSourceIn::create(
        const boost::intrusive_ptr<ExpressionContext>& pExpCtx,
        const std::shared_ptr<cppkafka::Consumer> consumer,
        const std::string topic,
        const std::string format) {
    boost::intrusive_ptr<DocumentSourceIn> source(new DocumentSourceIn(
        pExpCtx, consumer, topic, format));
    return source;
};


boost::intrusive_ptr<DocumentSource> DocumentSourceIn::createFromBson(
    BSONElement elem, const boost::intrusive_ptr<ExpressionContext>& pExpCtx) {

    // $stream should be creating this directly with DocumentSourceIn::create
    MONGO_UNREACHABLE;
}

}  // namespace mongo
