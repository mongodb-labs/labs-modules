#pragma once

#include <cppkafka/cppkafka.h>

#include "mongo/db/pipeline/document_source.h"

#include "source_connector.h"

namespace mongo {
  class KafkaSourceConnector : public SourceConnector {
    public:
      KafkaSourceConnector(
        const std::shared_ptr<cppkafka::Consumer> consumer,
        const std::string topic
      );

      DocumentSource::GetNextResult getNext() final;

      std::shared_ptr<cppkafka::Consumer> getConsumer();

    private:
      std::shared_ptr<cppkafka::Consumer> _consumer;
  };

} // namespace mongo
