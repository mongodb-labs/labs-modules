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

#include "document_source_semantic_search.h"
#include "mongo/db/modules/labs-modules/src/external/document_source_external.h"

#include "mongo/db/jsobj.h"
#include "mongo/db/pipeline/document_source_limit.h"
#include "mongo/db/pipeline/document_source_project.h"
#include "mongo/db/pipeline/document_source_unwind.h"
#include "mongo/db/pipeline/document_source_lookup.h"
#include "mongo/db/pipeline/document_source_replace_root.h"
#include "mongo/db/pipeline/document_source_add_fields.h"
#include "mongo/db/pipeline/document_source_group.h"
#include "mongo/db/pipeline/expression_context.h"
#include "mongo/db/pipeline/lite_parsed_document_source.h"

namespace mongo {

using boost::intrusive_ptr;
using std::list;
using std::string;

REGISTER_DOCUMENT_SOURCE(semanticSearch,
                         LiteParsedDocumentSourceDefault::parse,
                         DocumentSourceSemanticSearch::createFromBson,
                         AllowedWithApiStrict::kAlways);

list<intrusive_ptr<DocumentSource>> DocumentSourceSemanticSearch::createFromBson(
    BSONElement elem, const intrusive_ptr<ExpressionContext>& pExpCtx) {
    uassert(40156,
            str::stream() << "the semanticSearch field must be a non-empty string",
            elem.type() == String);

    StringData elemString = elem.valueStringData();
    uassert(
        40157, str::stream() << "the semanticSearch field must be a non-empty string", !elemString.empty());

    uassert(40158,
            str::stream() << "the semanticSearch field cannot be a $-prefixed path",
            elemString[0] != '$');

    uassert(40159,
            str::stream() << "the semanticSearch field cannot contain a null byte",
            elemString.find('\0') == string::npos);

    uassert(40160,
            str::stream() << "the semanticSearch field cannot contain '.'",
            elemString.find('.') == string::npos);

    BSONObj limitObj = BSON("$limit" << 1);
    BSONObj projectObj = BSON("$project" << BSON("_id" << 0 << "query" << BSON("$literal" << elemString) << "num_results" << 10));
    BSONObj externalObj = BSON("$external" << BSON("name" << "_internalSemanticSearch"));
    BSONObj unwindObj1 = BSON("$unwind" << "$response.matches");
    BSONObj groupObj = BSON("$group" << BSON("_id" << "$response.matches.docid" << "matches" << BSON("$push" << "$response.matches")));
    BSONObj unsetObj = BSON("$project" << BSON("matches.docid" << 0 << "matches.title" << 0));
    BSONObj lookupObj = BSON("$lookup" <<
                                BSON("from" << pExpCtx->ns.coll() << "localField" << "_id" << "foreignField" << "_id" << "as" << "docs"));

    BSONObj unwindObj2 = BSON("$unwind" << "$docs");
    BSONObj addFieldsObj = BSON("$addFields" << BSON("docs.__sem_search__" << "$matches"));
    BSONObj replaceRootObj = BSON("$replaceRoot" << BSON("newRoot" << "$docs"));

    // Need to explicitly add the namespace for $lookup
    LiteParsedPipeline liteParsedPipeline(pExpCtx->ns, {lookupObj});
    pExpCtx->addResolvedNamespaces(liteParsedPipeline.getInvolvedNamespaces());

    auto limitSource = DocumentSourceLimit::createFromBson(limitObj.firstElement(), pExpCtx);
    auto projectSource = DocumentSourceProject::createFromBson(projectObj.firstElement(), pExpCtx);
    auto externalSource = DocumentSourceExternal::createFromBson(externalObj.firstElement(), pExpCtx);
    auto unwindSource1 = DocumentSourceUnwind::createFromBson(unwindObj1.firstElement(), pExpCtx);
    auto groupSource = DocumentSourceGroup::createFromBson(groupObj.firstElement(), pExpCtx);
    auto unsetSource = DocumentSourceProject::createFromBson(unsetObj.firstElement(), pExpCtx);
    auto lookupSource = DocumentSourceLookUp::createFromBson(lookupObj.firstElement(), pExpCtx);
    auto unwindSource2 = DocumentSourceUnwind::createFromBson(unwindObj2.firstElement(), pExpCtx);
    auto addFieldsSource = DocumentSourceAddFields::createFromBson(addFieldsObj.firstElement(), pExpCtx);
    auto replaceRootSource = DocumentSourceReplaceRoot::createFromBson(replaceRootObj.firstElement(), pExpCtx);


    return { limitSource, projectSource, externalSource, unwindSource1, groupSource, unsetSource, lookupSource, unwindSource2, addFieldsSource, replaceRootSource };
}

}  // namespace mongo