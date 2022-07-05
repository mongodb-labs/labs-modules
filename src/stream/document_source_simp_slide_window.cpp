#define MONGO_LOGV2_DEFAULT_COMPONENT ::mongo::logv2::LogComponent::kDefault

#include "mongo/platform/basic.h"

#include "document_source_simp_slide_window.h"

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
#include "mongo/logv2/log.h"
#include "mongo/stdx/condition_variable.h"
#include "mongo/util/assert_util.h"
#include "mongo/util/str.h"

#include <iostream>

namespace mongo {

using boost::intrusive_ptr;
using std::cout;
using std::endl;
using std::pair;
using std::string;
using std::vector;

REGISTER_DOCUMENT_SOURCE(simpSWindow, LiteParsedDocumentSourceDefault::parse,
                         DocumentSourceSimpSWindow::createFromBson,
                         AllowedWithApiStrict::kAlways);

intrusive_ptr<DocumentSource> DocumentSourceSimpSWindow::createFromBson(
    BSONElement elem, const intrusive_ptr<ExpressionContext> &expCtx) {
  LOGV2(99999, "Create simpSWindow: ", "arg"_attr = elem);
  uassert(99999,
          str::stream() << "the duration field must be an obj, got: "
                        << typeName(elem.type()),
          elem.type() == BSONType::Object);
  auto elemObj = elem.Obj();
  uassert(99999, "arg must contain field `n`", elemObj.hasField("n"));
  uassert(99999, "arg must contain field `gap`", elemObj.hasField("gap"));

  int n = elemObj.getField("n").numberInt();
  int gap = elemObj.getField("gap").numberInt();

  boost::intrusive_ptr<DocumentSource> simpSWinStage(
      new DocumentSourceSimpSWindow(expCtx, n, gap));
  return simpSWinStage;
}

DocumentSource::GetNextResult DocumentSourceSimpSWindow::doGetNext() {
  cout << "simpswin get next" << endl;

  switch (_state) {
  case GET: {
    auto next = pSource->getNext();
    _nElem += 1;
    if (_nElem >= _nWindow &&
        (_nElem - _nWindow + _interval) % _interval == 0) {
      _state = WINDOW;
      _nElem -= _interval;
    }
    return next;
  }
  case WINDOW: {
    _state = POP;
    return DocumentSource::GetNextResult::makeUnblock(Document());
  }
  case POP: {
    _state = GET;
    return DocumentSource::GetNextResult::makePop({{"interval", _interval}});
  }
  }
  MONGO_UNREACHABLE;
}

} // namespace mongo
