#define MONGO_LOGV2_DEFAULT_COMPONENT ::mongo::logv2::LogComponent::kDefault

#include "mongo/bson/json.h"
#include "mongo/logv2/log.h"
#include "mongo/platform/basic.h"

#include "kafka_source_connector.h"

namespace mongo {

KafkaSourceConnector::KafkaSourceConnector (
  const std::shared_ptr<cppkafka::Consumer> consumer,
  const std::string topic
): _consumer(consumer) {
  // Setting source connector type
  _type = SourceConnector::Type::kKafka;

  // Subscribe to the topic
  _consumer->subscribe({ topic });
}

std::shared_ptr<cppkafka::Consumer> KafkaSourceConnector::getConsumer() {
  return _consumer;
};

DocumentSource::GetNextResult KafkaSourceConnector::getNext() {
  if (!_consumer) {
    return DocumentSource::GetNextResult::makePauseExecution();
  }

	cppkafka::Message msg = _consumer->poll();

  // If we managed to get a message
  if (msg) {
      if (msg.is_eof()) {
          // EOF means we've reached the end of the log
          LOGV2(999999, "Received Kafka EOF");
          return DocumentSource::GetNextResult::makeEOF();
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
              return DocumentSource::GetNextResult::makePauseExecution();
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
          return DocumentSource::DocumentSource::GetNextResult(std::move(outputDoc));
      }
    }

    return DocumentSource::GetNextResult::makePauseExecution();
};

}