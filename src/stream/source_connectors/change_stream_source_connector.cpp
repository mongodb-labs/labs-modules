#define MONGO_LOGV2_DEFAULT_COMPONENT ::mongo::logv2::LogComponent::kDefault

#include "mongo/platform/basic.h"

#include "mongo/db/commands/run_aggregate.h"
#include "mongo/db/db_raii.h"
#include "mongo/db/pipeline/aggregation_request_helper.h"
#include "mongo/db/repl/replication_coordinator.h"
#include "mongo/db/tenant_id.h"
#include "mongo/logv2/log.h"

#include "change_stream_source_connector.h"

namespace mongo {

ChangeStreamSourceConnector::ChangeStreamSourceConnector(
  const boost::intrusive_ptr<ExpressionContext>& expCtx,
  const std::string db,
  const std::string coll) : _expCtx(expCtx) {
    boost::optional<TenantId> tenantId;
    _nss = NamespaceString(tenantId, db, coll);

    try {
        _buildPipeline();
    } catch (const std::exception& e) {
        LOGV2_ERROR(999999, "error building pipeline", "error"_attr = redact(e.what()));
    }
  }

DocumentSource::GetNextResult ChangeStreamSourceConnector::getNext() {
  if (!_exec) {
    return DocumentSource::GetNextResult::makePauseExecution();
  }

  BSONObj obj;

  auto state = _exec->getNext(&obj, nullptr);

  if (PlanExecutor::ADVANCED == state) {
      LOGV2(999999, "Received change stream", "payload"_attr = obj);

      // Create new Document from newly constructed BSONObj and return
      return DocumentSource::DocumentSource::GetNextResult(Document(std::move(obj)));
  }

  return DocumentSource::GetNextResult::makePauseExecution();
};

void ChangeStreamSourceConnector::_buildPipeline() {

  std::vector<BSONObj> changeStreamPipeline { BSON("$changeStream" << BSONObj()) };

  auto changeStreamAggregation = BSON("aggregate" << _nss.coll() << "pipeline" << changeStreamPipeline << "readConcern" << BSON("level" << "majority") << "cursor" << BSONObj());
  auto changeStreamAggCmd =
      OpMsgRequest::fromDBAndBody(_nss.db(), changeStreamAggregation).body;

  auto opCtx = _expCtx->opCtx;
  auto recovUnit = opCtx->releaseAndReplaceRecoveryUnit();

  auto changeStreamAggRequest = aggregation_request_helper::parseFromBSON(
      opCtx,
      _nss,
      changeStreamAggCmd,
      boost::none,
      false);

  _exec = getPlanExecutorFromAggCmd(
                opCtx,
                changeStreamAggRequest.getNamespace(),
                changeStreamAggRequest,
                {changeStreamAggRequest},
                changeStreamAggregation);

  opCtx->setRecoveryUnit(std::move(recovUnit),
                                WriteUnitOfWork::RecoveryUnitState::kNotInUnitOfWork);

};

} //namespace mongo