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

#include <deque>
#include <memory>
#include <utility>

#include "mongo/db/pipeline/accumulation_statement.h"
#include "mongo/db/pipeline/accumulator.h"
#include "mongo/db/pipeline/document_source.h"
#include "mongo/db/pipeline/document_source_group.h"
#include "mongo/db/pipeline/memory_usage_tracker.h"
#include "mongo/db/pipeline/transformer_interface.h"

#include "../utils/doc_pause.h"
#include "../utils/mk_eof.h"

namespace mongo {

class DocumentSourceGroupStream final : public DocumentSource {
public:
  static constexpr StringData kStageName = "$groups"_sd;

  boost::intrusive_ptr<DocumentSource> optimize() final { return this; }
  const char *getSourceName() const final { return kStageName.rawData(); }
  // DepsTracker::State getDependencies(DepsTracker *deps) const final;
  Value serialize(boost::optional<ExplainOptions::Verbosity> explain =
                      boost::none) const final {
    // TODO: this is a stub
    return Value(Document{{"$groups", Value(1)}});
  }

  /**
   * Parses 'elem' into a $group stage, or throws a AssertionException if 'elem'
   * was an invalid specification.
   */
  static boost::intrusive_ptr<DocumentSource>
  createFromBson(BSONElement elem,
                 const boost::intrusive_ptr<ExpressionContext> &expCtx);

  StageConstraints constraints(Pipeline::SplitState pipeState) const final {
    StageConstraints constraints(
        StreamType::kBlocking, PositionRequirement::kNone,
        HostTypeRequirement::kNone, DiskUseRequirement::kWritesTmpData,
        FacetRequirement::kAllowed, TransactionRequirement::kAllowed,
        LookupRequirement::kAllowed, UnionRequirement::kAllowed);
    constraints.canSwapWithMatch = true;
    return constraints;
  }

  boost::optional<DistributedPlanLogic> distributedPlanLogic() final {
    return boost::none;
  }

  bool canRunInParallelBeforeWriteStage(
      const std::set<std::string> &nameOfShardKeyFieldsUponEntryToStage)
      const final {
    return false;
  };

  // True if this $group can be pushed down to SBE.
  bool sbeCompatible() const { return false; }

protected:
  GetNextResult doGetNext() final;
  void doDispose() final;

private:
  explicit DocumentSourceGroupStream(
      const boost::intrusive_ptr<ExpressionContext> &expCtx, BSONElement spec);

  /**
   * Before returning anything, this source must prepare itself. In a streaming
   * $group, initialize() requests the first document from the previous source,
   * and uses it to prepare the accumulators. In an unsorted $group,
   * initialize() exhausts the previous source before returning. The
   * '_initialized' boolean indicates that initialize() has finished.
   *
   * This method may not be able to finish initialization in a single call if
   * 'pSource' returns a DocumentSource::GetNextResult::kPauseExecution, so it
   * returns the last GetNextResult encountered, which may be either kEOF or
   * kPauseExecution.
   */

  void mergeStates();
  BSONElement _spec;
  std::deque<boost::intrusive_ptr<DocumentSource>> _innerGroups;
  boost::optional<boost::intrusive_ptr<DocumentSource>> _mergerStage;

  bool _eof;
  bool _windowed;
  bool _windowChanged;
};

} // namespace mongo
