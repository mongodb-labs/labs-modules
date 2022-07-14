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

#include "mongo/bson/json.h"
#include "mongo/db/commands/stream_registry.h"
#include "mongo/db/pipeline/pipeline.h"
#include "mongo/logv2/log.h"
#include "mongo/platform/basic.h"

#include "document_source_in.h"

namespace mongo {

DocumentSourceIn::DocumentSourceIn(
    const boost::intrusive_ptr<ExpressionContext>& pExpCtx,
    const std::shared_ptr<SourceConnector> sourceConnector,
    const std::string streamName)
    : DocumentSource(kStageName, pExpCtx),
    _sourceConnector(sourceConnector),
    _manualInsertionSourceConnector(new ManualInsertionSourceConnector(streamName)) {}


// Macro to register the document source.
REGISTER_DOCUMENT_SOURCE(in,
                         LiteParsedDocumentSourceDefault::parse,
                         DocumentSourceIn::createFromBson,
                         AllowedWithApiStrict::kAlways);


DocumentSource::GetNextResult DocumentSourceIn::doGetNext() {
    if (_manualInsertionSourceConnector) {
        auto doc = _manualInsertionSourceConnector->getNext();
        if (!doc.isPaused()) {
            return doc;
        }
    }

    if (!_sourceConnector) {
        return GetNextResult::makePauseExecution();
    }

    return _sourceConnector->getNext();
}

Value DocumentSourceIn::serialize(boost::optional<ExplainOptions::Verbosity> explain) const {
    MutableDocument insides;

    // TODO.

    return Value{Document{{getSourceName(), insides.freezeToValue()}}};
}


boost::intrusive_ptr<DocumentSourceIn> DocumentSourceIn::create(
        const boost::intrusive_ptr<ExpressionContext>& pExpCtx,
        const std::shared_ptr<SourceConnector> sourceConnector,
        const std::string streamName) {
    boost::intrusive_ptr<DocumentSourceIn> source(new DocumentSourceIn(
        pExpCtx, sourceConnector, streamName));
    return source;
};


boost::intrusive_ptr<DocumentSource> DocumentSourceIn::createFromBson(
    BSONElement elem, const boost::intrusive_ptr<ExpressionContext>& pExpCtx) {

    // $stream should be creating this directly with DocumentSourceIn::create
    MONGO_UNREACHABLE;
}

}  // namespace mongo
