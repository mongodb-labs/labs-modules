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

#include "mongo/platform/basic.h"

#include "document_source_group_stream.h"

#include <memory>

#include "mongo/db/exec/document_value/document.h"
#include "mongo/db/exec/document_value/value.h"
#include "mongo/db/exec/document_value/value_comparator.h"
#include "mongo/db/pipeline/accumulation_statement.h"
#include "mongo/db/pipeline/accumulator.h"
#include "mongo/db/pipeline/expression.h"
#include "mongo/db/pipeline/expression_context.h"
#include "mongo/db/pipeline/lite_parsed_document_source.h"
#include "mongo/db/stats/resource_consumption_metrics.h"
#include "mongo/util/destructor_guard.h"

#include <chrono>
#include <cstdlib>
#include <string>
#include <thread>
#include <vector>

namespace mongo {

using boost::intrusive_ptr;
using std::pair;
using std::shared_ptr;
using std::vector;

REGISTER_DOCUMENT_SOURCE(groups, LiteParsedDocumentSourceDefault::parse,
                         DocumentSourceGroupStream::createFromBson,
                         AllowedWithApiStrict::kAlways);

DocumentSource::GetNextResult DocumentSourceGroupStream::doGetNext() {
  if (_windowed) {
    if (_windowChanged) {
      mergeStates();
    }
    invariant(!_windowChanged);
    invariant(_mergerStage);
    auto res = (*_mergerStage)->getNext();

    if (res.isEOF()) {
      if (_eof) {
        dispose(); // really done, propagete disposing up
        return res;
      }
      doDispose();
      return DocumentSource::GetNextResult::makePauseExecution();
    }
    return res;
  } else {
    auto next = pSource->getNext();
    while (next.isAdvanced()) {
      auto doc = next.releaseDocument();
      _innerGroups.front()->setSource(new DocPause(pExpCtx, doc));
      _innerGroups.front()->getNext();
      _windowChanged = true;
      next = pSource->getNext();
    }
    switch (next.getStatus()) {
    case DocumentSource::GetNextResult::ReturnStatus::kAdvanced: {
      MONGO_UNREACHABLE;
    }
    case DocumentSource::GetNextResult::ReturnStatus::kPop: {
      _innerGroups.pop_back();
      _windowChanged = true;
      return DocumentSource::GetNextResult::makePauseExecution();
    }
    case DocumentSource::GetNextResult::ReturnStatus::kPartial: {
      auto inner = DocumentSourceGroup::createFromBson(_spec, pExpCtx);
      inner->setRevisitable(true);
      _innerGroups.push_front(inner);
      return DocumentSource::GetNextResult::makePauseExecution();
    }
    case DocumentSource::GetNextResult::ReturnStatus::kEOF: {
      _eof = true;
      [[fallthrough]];
    }
    case DocumentSource::GetNextResult::ReturnStatus::kUnblock: {
      _windowed = true;
      // return next;
      return DocumentSource::GetNextResult::makePauseExecution();
    }
    default: {
      return next;
    }
    }
  }
}

void DocumentSourceGroupStream::doDispose() {
  // reset states
  _windowed = false;
  _mergerStage = boost::none;
}

// DepsTracker::State
// DocumentSourceGroupStream::getDependencies(DepsTracker *deps) const {
//   // add the _id
//   for (size_t i = 0; i < _idExpressions.size(); i++) {
//     _idExpressions[i]->addDependencies(deps);
//   }

//   // add the rest
//   for (auto &&accumulatedField : _accumulatedFields) {
//     accumulatedField.expr.argument->addDependencies(deps);
//     // Don't add initializer, because it doesn't refer to docs from the input
//     // stream.
//   }

//   return DepsTracker::State::EXHAUSTIVE_ALL;
// }

DocumentSourceGroupStream::DocumentSourceGroupStream(
    const intrusive_ptr<ExpressionContext> &expCtx, BSONElement spec)
    : DocumentSource(kStageName, expCtx), _spec(spec), _innerGroups{},
      _mergerStage(boost::none), _eof(false), _windowed(false),
      _windowChanged(true) {
  auto inner = DocumentSourceGroup::createFromBson(_spec, pExpCtx);
  inner->setRevisitable(true);
  _innerGroups.push_front(inner);
}

intrusive_ptr<DocumentSource> DocumentSourceGroupStream::createFromBson(
    BSONElement elem, const intrusive_ptr<ExpressionContext> &expCtx) {
  uassert(15947, "a group's fields must be specified in an object",
          elem.type() == Object);
  boost::intrusive_ptr<DocumentSource> res(
      new DocumentSourceGroupStream(expCtx, elem));
  return res;
}

void DocumentSourceGroupStream::mergeStates() {
  auto d = _innerGroups.front()->distributedPlanLogic();
  boost::intrusive_ptr<DocumentSource> merger = (*d).mergingStages.front();
  for (auto &grp : _innerGroups) {
    grp->setSource(new MkEOF(pExpCtx));
    auto next = grp->getNext();
    while (!next.isEOF()) {
      if (!next.isAdvanced()) {
        next = grp->getNext();
        continue;
      }
      auto doc = next.releaseDocument();
      merger->setSource(new DocPause(pExpCtx, doc));
      merger->getNext();
      next = grp->getNext();
    }
  }
  merger->setSource(new MkEOF(pExpCtx));
  _mergerStage = merger;
  _windowChanged = false;
}

} // namespace mongo
