#pragma once

#include "mongo/db/pipeline/document_source.h"

namespace mongo {
  // Abstract class for source connectors
  // Very thin class at the moment - there's room to add more functionality
  // Allows for polymorphism in $in stage

  class SourceConnector {
    public:
      enum class Type {
        // Manual insertion conncector
        kManualInsertion,
        // Kafka connector
        kKafka
      };

      virtual ~SourceConnector() = default;

      // Get next value from source connector
      virtual DocumentSource::GetNextResult getNext() = 0;

      virtual SourceConnector::Type getType() {
        return _type;
      };

    protected:
      SourceConnector::Type _type;
  };

} // namespace mongo