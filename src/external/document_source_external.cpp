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

#include "mongo/bson/json.h"
#include "document_source_external.h"
#include "registry/registry.h"
#include "mongo/logv2/log.h"
#include "mongo/platform/basic.h"
#include "mongo/util/net/http_client.h"

namespace mongo {

DocumentSourceExternal::DocumentSourceExternal(
    const boost::intrusive_ptr<ExpressionContext>& pExpCtx,
    Endpoint* endpoint)
    : DocumentSource(kStageName, pExpCtx),
    _endpoint(endpoint) {}


// Macro to register the document source.
REGISTER_DOCUMENT_SOURCE(external,
                         LiteParsedDocumentSourceDefault::parse,
                         DocumentSourceExternal::createFromBson,
                         AllowedWithApiStrict::kAlways);

DocumentSource::GetNextResult DocumentSourceExternal::doGetNext() {
    auto nextInput = pSource->getNext();

    if (nextInput.isEOF()) {
        return GetNextResult::makeEOF();
    }

    if (nextInput.isPaused()) {
        return GetNextResult::makePauseExecution();
    }

    // Get Document and associated BSON
    Document currentDoc = nextInput.getDocument();
    BSONObj currentObj = currentDoc.toBson();

    // Send BSON to endpoint
    BSONObj obj = _endpoint->sendHttpRequest(currentObj);

    // Create new BSON with response
    BSONObjBuilder builder = BSONObjBuilder();
    builder.appendElements(currentObj);
    builder.append(_endpoint->getAs(), obj);

    BSONObj newObj = builder.done();

    // Create new Document from newly constructed BSONObj and return
    return DocumentSource::GetNextResult(Document(newObj));
}

Value DocumentSourceExternal::serialize(boost::optional<ExplainOptions::Verbosity> explain) const {
    MutableDocument insides;

    // TODO.

    return Value{Document{{getSourceName(), insides.freezeToValue()}}};
}


boost::intrusive_ptr<DocumentSourceExternal> DocumentSourceExternal::create(
    const boost::intrusive_ptr<ExpressionContext>& pExpCtx,
    Endpoint* endpoint) {
    boost::intrusive_ptr<DocumentSourceExternal> source(new DocumentSourceExternal(
        pExpCtx, endpoint));
    return source;
};


boost::intrusive_ptr<DocumentSource> DocumentSourceExternal::createFromBson(
    BSONElement elem, const boost::intrusive_ptr<ExpressionContext>& pExpCtx) {

    uassert(100029201,
            str::stream() << "The argument to $external must be an object, but found type: "
                          << typeName(elem.type()),
            elem.type() == BSONType::Object);

    BSONObj argObj = elem.Obj();
    auto name = argObj.getField("name").str();

    uassert(100029207,
        "$external requires 'name' to be specified", !name.empty());

    auto endpointObj = EndpointRegistry::get()->getEndpoint(name);

    uassert(100029211,
            str::stream() << "There is no endpoint registered with the name: " << name,
            endpointObj);

    return DocumentSourceExternal::create(pExpCtx,endpointObj);
}

}  // namespace mongo
