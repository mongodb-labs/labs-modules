#include "mongo/platform/basic.h"

#include "document_source_exp_window.h"

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

REGISTER_DOCUMENT_SOURCE(expWindow, LiteParsedDocumentSourceDefault::parse,
                         DocumentSourceExpWindow::createFromBson,
                         AllowedWithApiStrict::kAlways);

intrusive_ptr<DocumentSource> DocumentSourceExpWindow::createFromBson(
    BSONElement elem, const intrusive_ptr<ExpressionContext> &expCtx) {
  uassert(99999, str::stream() << "the count field must be an integer",
          elem.type() == NumberInt);
  auto winSize = elem.numberInt();
  intrusive_ptr<DocumentSourceExpWindow> expWinStage(
      new DocumentSourceExpWindow(expCtx, winSize));
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

DocumentSource::GetNextResult DocumentSourceExpWindow::doGetNext() {
  if (_n_past >= _window_size) {
    _n_past = 0;
    return GetNextResult::makeUnblock(Document());
  }
  auto res = pSource->getNext();
  if (res.getStatus() ==
      DocumentSource::GetNextResult::ReturnStatus::kAdvanced) {
    _n_past++;
  }
  return res;
}

} // namespace mongo
