#define MONGO_LOGV2_DEFAULT_COMPONENT ::mongo::logv2::LogComponent::kDefault

#include "mongo/platform/basic.h"

#include "document_source_simp_time_window.h"

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

REGISTER_DOCUMENT_SOURCE(simpTWindow, LiteParsedDocumentSourceDefault::parse,
                         DocumentSourceSimpTWindow::createFromBson,
                         AllowedWithApiStrict::kAlways);

intrusive_ptr<DocumentSource> DocumentSourceSimpTWindow::createFromBson(
    BSONElement elem, const intrusive_ptr<ExpressionContext> &expCtx) {
  LOGV2(99999, "Create simpTWindow: ", "arg"_attr = elem);
  const auto dura = elem.parseIntegerElementToNonNegativeLong();
  uassert(99999,
          str::stream() << "the duration field must be an integer, got: "
                        << typeName(elem.type()),
          dura.isOK());
  auto duration = dura.getValue();
  auto runner = makePeriodicRunner(expCtx->opCtx->getServiceContext());
  intrusive_ptr<DocumentSourceSimpTWindow> simpTWinStage(
      new DocumentSourceSimpTWindow(expCtx, duration, std::move(runner)));
  simpTWinStage->startTimer();
  return simpTWinStage;
}

void DocumentSourceSimpTWindow::startTimer() {
  cout << "starting timer" << endl;
  _timer = _timerRunner->makeJob(PeriodicRunner::PeriodicJob(
      "simpTWindow timer",
      [this](Client *) {
        if (!_inited)
          return;
        _windowed.store(true, std::memory_order_seq_cst);
        cout << "closed a window" << endl;
      },
      Milliseconds(_duration)));
  _timer.start();
}

void DocumentSourceSimpTWindow::getNext() {
  _next =
      std::async(std::launch::async, [this]() { return pSource->getNext(); });
}

template <typename T> inline bool isReady(std::future<T> const &f) {
  return f.valid() &&
         f.wait_until(std::chrono::system_clock::time_point::min()) ==
             std::future_status::ready;
}

DocumentSource::GetNextResult DocumentSourceSimpTWindow::doGetNext() {
  cout << "simptwin get next" << endl;
  if (!_inited) {
    getNext();
    _inited = true;
  }
  while (!isReady(_next)) {
    if (_windowed.load(std::memory_order_seq_cst)) {
      _windowed.store(false, std::memory_order_seq_cst);
      return DocumentSource::GetNextResult::makeUnblock(Document());
    }
  }
  auto next = _next.get();
  if (!next.isEOF()) {
    getNext();
  }
  return next;
}

} // namespace mongo
