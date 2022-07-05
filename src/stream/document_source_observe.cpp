#include "mongo/platform/basic.h"

#include "document_source_observe.h"

#include <memory>
#include <vector>

#include "mongo/base/string_data.h"
#include "mongo/bson/bsonobj.h"
#include "mongo/bson/bsonobjbuilder.h"
#include "mongo/bson/bsontypes.h"
#include "mongo/bson/oid.h"
#include "mongo/db/exec/document_value/document.h"
#include "mongo/db/exec/document_value/value.h"
#include "mongo/db/pipeline/document_source_tee_consumer.h"
#include "mongo/db/pipeline/expression_context.h"
#include "mongo/db/pipeline/field_path.h"
#include "mongo/db/pipeline/pipeline.h"
#include "mongo/db/pipeline/tee_buffer.h"
#include "mongo/util/assert_util.h"
#include "mongo/util/str.h"

namespace mongo {

using boost::intrusive_ptr;
using std::pair;
using std::string;
using std::vector;

REGISTER_DOCUMENT_SOURCE(ob, LiteParsedDocumentSourceDefault::parse,
                         DocumentSourceOb::createFromBson,
                         AllowedWithApiStrict::kAlways);

intrusive_ptr<DocumentSource> DocumentSourceOb::createFromBson(
    BSONElement elem, const intrusive_ptr<ExpressionContext> &expCtx) {
  intrusive_ptr<DocumentSourceOb> expWinStage(new DocumentSourceOb(expCtx));
  return expWinStage;
}

/*
DocumentSource::GetNextResult DocumentSourceExpWindow::initialize() {
    GetNextResult input = pSource->getNext();
    _acc_docs.push_back(input.releaseDocument());
    for (; input.isAdvanced() && _acc_docs.size() < _window_size; input =
pSource->getNext()) { _acc_docs.push_back(input.releaseDocument());
    }
    return input;
}
*/

DocumentSource::GetNextResult DocumentSourceOb::doGetNext() {
  auto res = pSource->getNext();
  switch (res.getStatus()) {
  case DocumentSource::GetNextResult::ReturnStatus::kAdvanced: {
    auto doc = res.releaseDocument();
    return Document({{"type", "kAdvanced"_sd}, {"data", doc}});
  }
  case DocumentSource::GetNextResult::ReturnStatus::kPauseExecution: {
    return Document({{"type", "kPauseExecution"_sd}});
  }
  case DocumentSource::GetNextResult::ReturnStatus::kUnblock: {
    return Document({{"type", "kUnblock"_sd}});
  }
  case DocumentSource::GetNextResult::ReturnStatus::kPop: {
    auto doc = res.releaseDocument();
    return Document({{"type", "kPop"_sd}, {"data", doc}});
  }
  case DocumentSource::GetNextResult::ReturnStatus::kEOF: {
    return res;
  }
  }
  MONGO_UNREACHABLE;
}

} // namespace mongo
