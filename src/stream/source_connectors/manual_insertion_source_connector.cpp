#define MONGO_LOGV2_DEFAULT_COMPONENT ::mongo::logv2::LogComponent::kDefault

#include "mongo/db/commands/stream_registry.h"
#include "mongo/logv2/log.h"

#include "manual_insertion_source_connector.h"

namespace mongo {

ManualInsertionSourceConnector::ManualInsertionSourceConnector(const std::string streamName) {
  // Setting source connector type
  _type = SourceConnector::Type::kManualInsertion;

  std::unique_ptr<StreamListener> listener = std::make_unique<StreamListener>([this](DocumentSource::GetNextResult document) {
      stdx::lock_guard<Latch> lock(_mutex);
      _insertionQueue.push_back(std::move(document));
  });

  auto stream = StreamRegistry::get()->getStream(streamName);
  stream->addListener(std::move(listener));
}

DocumentSource::GetNextResult ManualInsertionSourceConnector::getNext() {
  if (auto entry = _popInsertionQueueIfNotEmpty()) {
        auto entryObj = entry.get();
        LOGV2(999999, "Received manual insertion", "payload"_attr = entryObj.getDocument().toBson());
        return entryObj;
  }
  return DocumentSource::GetNextResult::makePauseExecution();
}

boost::optional<DocumentSource::GetNextResult> ManualInsertionSourceConnector::_popInsertionQueueIfNotEmpty() {
    // Protect _insertionQueue
    stdx::lock_guard<Latch> lock(_mutex);
    if (_insertionQueue.size() > 0) {
        auto out = std::move(_insertionQueue.front());
        _insertionQueue.pop_front();
        return out;
    }
    return boost::none;
}

} // namespace mongo