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

#define MONGO_LOGV2_DEFAULT_COMPONENT ::mongo::logv2::LogComponent::kDefault

#include <boost/algorithm/string.hpp>
#include <boost/intrusive_ptr.hpp>

#include "mongo/bson/json.h"
#include "mongo/logv2/log.h"
#include "mongo/platform/basic.h"

#include "document_source_in.h"
#include "document_source_stream.h"
#include "document_source_stream_commit.h"
#include "document_source_stream_controller.h"
#include "source_connectors/change_stream_source_connector.h"
#include "source_connectors/kafka_source_connector.h"
#include "source_connectors/source_connector.h"

namespace mongo {

DocumentSourceStream::DocumentSourceStream(
    const boost::intrusive_ptr<ExpressionContext> &pExpCtx,
    std::unique_ptr<Pipeline, PipelineDeleter> pipeline,
    const BSONObj metadata,
    const std::shared_ptr<SourceConnector> sourceConnector,
    const std::shared_ptr<cppkafka::Consumer> consumer)
    : DocumentSource(kStageName, pExpCtx),
    _pipeline(std::move(pipeline)),
    _streamController(new StreamController()) {
      auto streamName = metadata.getField("name").str();

      // Insert $streamController as initial source
      _streamController->setSource(DocumentSourceIn::create(
          pExpCtx, sourceConnector, streamName));

      _pipeline->addInitialSource(
          DocumentSourceStreamController::create(pExpCtx, _streamController));

      /*
      * Add $streamCommit to end
      * Agg pipeline shoud look like:
      * [$in] => [$streamController] => [Regular Agg] => [$streamCommit]
      * Previous:
      * [$in] => [Regular Agg]
      */

      if (sourceConnector != nullptr) {
        if (sourceConnector->getType() == SourceConnector::Type::kKafka) {
          _pipeline->addFinalSource(
            DocumentSourceStreamCommit::create(pExpCtx, consumer));
        }
      }
}

// Macro to register the document source.
REGISTER_DOCUMENT_SOURCE(stream, LiteParsedDocumentSourceDefault::parse,
                         DocumentSourceStream::createFromBson,
                         AllowedWithApiStrict::kAlways);

DocumentSource::GetNextResult DocumentSourceStream::doGetNext() {
  bool pipelineEOF = false;

  while (!pipelineEOF) {
    auto input = _pipeline->getSources().back()->getNext();

    if (input.isShutdown()) {
      // Shutdown means we can exit the aggregation pipeline
      break;
    }

    pipelineEOF = input.isEOF();
  }

  return GetNextResult::makeEOF();
}

Value DocumentSourceStream::serialize(
    boost::optional<ExplainOptions::Verbosity> explain) const {
  MutableDocument insides;

  // TODO.

  return Value{Document{{getSourceName(), insides.freezeToValue()}}};
}

boost::intrusive_ptr<DocumentSourceStream> DocumentSourceStream::create(
    const boost::intrusive_ptr<ExpressionContext> &pExpCtx,
    std::unique_ptr<Pipeline, PipelineDeleter> pipeline,
    const BSONObj metadata,
    const std::shared_ptr<SourceConnector> sourceConnector,
    const std::shared_ptr<cppkafka::Consumer> consumer) {
  boost::intrusive_ptr<DocumentSourceStream> source(new DocumentSourceStream(
      pExpCtx, std::move(pipeline), metadata, sourceConnector, consumer));
  return source;
};

boost::intrusive_ptr<DocumentSource> DocumentSourceStream::createFromBson(
    BSONElement elem, const boost::intrusive_ptr<ExpressionContext> &pExpCtx) {

  LOGV2(999999, "Creating $stream stage");

  uassert(999999,
          str::stream() << "arguments to $stream must be an object, is type "
                        << typeName(elem.type()),
          elem.type() == BSONType::Object);

  auto elemObj = elem.Obj();

  uassert(999999,
          str::stream() << "pipeline field not set.",
          elemObj.hasField("pipeline"));

  uassert(999999,
          str::stream() << "metadata field not set.",
          elemObj.hasField("metadata"));

  auto metadataObj = elemObj.getField("metadata").Obj();

  std::vector<BSONObj> rawPipeline;
  std::shared_ptr<SourceConnector> sourceConnector;
  std::shared_ptr<cppkafka::Consumer> kafkaConsumer;


  for (auto &&subPipeElem : elemObj.getField("pipeline").Obj()) {
    uassert(
        99999999,
        str::stream() << "elements of arrays in $stream spec must be non-empty "
                         "objects, argument contained an element of type "
                      << typeName(subPipeElem.type()) << ": " << subPipeElem,
        subPipeElem.type() == BSONType::Object);

    auto embeddedObject = subPipeElem.embeddedObject();

    if (embeddedObject.hasField("$in")) {
      BSONElement inArgElem = subPipeElem.Obj().getField("$in");
      BSONObj argObj = inArgElem.Obj();

      if (argObj.hasField("db") && argObj.hasField("coll")) {
        std::string db = argObj.getField("db").str();
        std::string coll = argObj.getField("coll").str();

        sourceConnector = std::make_shared<ChangeStreamSourceConnector>(pExpCtx, db, coll);
      } else if (argObj.hasField("connectionConfig")) {

        auto connectionConfig = argObj.getField("connectionConfig");

        uassert(100029201,
                str::stream() << "The connectionConfig argument to $stream must "
                                "be an object, but found type: "
                              << typeName(elem.type()),
                connectionConfig.type() == BSONType::Object);

        auto connectionConfigObj = connectionConfig.Obj();

        // TODO: Need to do additional input validation for each of these steps.
        auto bootstrapServer =
            connectionConfigObj.getField("bootstrapServer").str();
        auto streamUUID = metadataObj.getField("id").str();
        std::string kafkaTopic = connectionConfigObj.getField("topic").str();
        std::string kafkaTopicFormat = connectionConfigObj.getField("format").str();

        LOGV2(999999, "streamUUID", "streamUUID"_attr=streamUUID);

        uassert(999999,
            str::stream() << "required bootstrapServer or kafkaTopic field not set.",
            (!bootstrapServer.empty() && !kafkaTopic.empty()));

        cppkafka::Configuration kafkaConfig = {{"bootstrap.servers", bootstrapServer},
                      // Change to catalog UUID once we have this
                      {"group.id", streamUUID},
                      // Disable auto commit
                      {"enable.auto.commit", false},
                      {"auto.offset.reset", "beginning"}};

        kafkaConsumer = std::make_shared<cppkafka::Consumer>(kafkaConfig);
        sourceConnector = std::make_shared<KafkaSourceConnector>(kafkaConsumer, kafkaTopic);
      }
    } else {
      // Do not insert $in as it will be added in manually
      rawPipeline.push_back(embeddedObject);
    }
  }

  auto pipeline =
      Pipeline::parse(rawPipeline, pExpCtx, [](const Pipeline &pipeline) {
        auto sources = pipeline.getSources();
        std::for_each(sources.begin(), sources.end(), [](auto &stage) {
          // TODO: Need to add in a isAllowedInStream stage constraint

          // auto stageConstraints = stage->constraints();

          // uassert(40600,
          //         str::stream() << stage->getSourceName()
          //                         << " is not allowed to be used within a
          //                         $facet stage",
          //         stageConstraints.isAllowedInChangeStream());
          // // We expect a stage within a $facet stage to have these
          // properties. invariant(stageConstraints.requiredPosition ==
          //             StageConstraints::PositionRequirement::kNone);
          // invariant(!stageConstraints.isIndependentOfAnyCollection);
        });
      });

  return DocumentSourceStream::create(pExpCtx, std::move(pipeline), std::move(metadataObj), sourceConnector, kafkaConsumer);
}

} // namespace mongo
