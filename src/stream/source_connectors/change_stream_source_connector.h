#pragma once

#include "mongo/db/namespace_string.h"
#include "mongo/db/pipeline/document_source.h"

#include "source_connector.h"

namespace mongo {
  class ChangeStreamSourceConnector : public SourceConnector {
    public:
      ChangeStreamSourceConnector(
        const boost::intrusive_ptr<ExpressionContext>& expCtx,
        const std::string db,
        const std::string coll
      );

      DocumentSource::GetNextResult getNext() final;

    private:
      void _buildPipeline();

      PlanExecutor* _getExecutor() const {
        return _exec.get();
      }

      std::unique_ptr<PlanExecutor, PlanExecutor::Deleter> _exec;
      const boost::intrusive_ptr<ExpressionContext>& _expCtx;

      NamespaceString _nss;
  };
}
