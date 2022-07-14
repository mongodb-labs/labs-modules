#pragma once

#include <cppkafka/cppkafka.h>

#include "mongo/db/pipeline/document_source.h"

#include "source_connector.h"

namespace mongo {
  class ManualInsertionSourceConnector : public SourceConnector {
    public:
      ManualInsertionSourceConnector(const std::string streamName);

      DocumentSource::GetNextResult getNext() final;

    private:
      boost::optional<DocumentSource::GetNextResult> _popInsertionQueueIfNotEmpty();

      mutable Mutex _mutex = MONGO_MAKE_LATCH("ManualInsertionSourceConnector::_mutex");
      std::deque<DocumentSource::GetNextResult> _insertionQueue;
  };
}
