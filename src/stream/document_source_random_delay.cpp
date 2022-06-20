#include "mongo/platform/basic.h"

#include "mongo/db/pipeline/document_source_random_delay.h"

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

#include <chrono>
#include <cstdlib>
#include <thread>
#include <iostream>

namespace mongo {

using boost::intrusive_ptr;
using std::pair;
using std::string;
using std::vector;


REGISTER_DOCUMENT_SOURCE(randomDelay,
                         LiteParsedDocumentSourceDefault::parse,
                         DocumentSourceRandomDelay::createFromBson,
                         AllowedWithApiStrict::kAlways);

intrusive_ptr<DocumentSource> DocumentSourceRandomDelay::createFromBson(
    BSONElement elem, const intrusive_ptr<ExpressionContext>& expCtx) {
    uassert(99999, str::stream() << "the max delay field must be an integer", elem.type() == NumberInt);
    auto maxDelay = elem.numberInt();
    intrusive_ptr<DocumentSourceRandomDelay> expWinStage(
        new DocumentSourceRandomDelay(expCtx, maxDelay));
    return expWinStage;
}

/*
DocumentSource::GetNextResult DocumentSourceExpWindow::initialize() {
    GetNextResult input = pSource->getNext();
    _acc_docs.push_back(input.releaseDocument());
    for (; input.isAdvanced() && _acc_docs.size() < _window_size; input = pSource->getNext()) {
        _acc_docs.push_back(input.releaseDocument());
    }
    return input;
}
*/

DocumentSource::GetNextResult DocumentSourceRandomDelay::doGetNext() {
    auto delay = std::rand() / ((RAND_MAX + 1u) / _max_delay);
    std::cout << "Delay for: " << delay << "ms" << std::endl;
    auto startTime = std::chrono::high_resolution_clock::now();
    std::this_thread::sleep_for(std::chrono::milliseconds{delay});
    auto endTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> delayed = endTime - startTime;
    std::cout << "Delayed: " << delayed.count() << "ms" << std::endl;
    auto res = pSource->getNext();
    return res;
}


}  // namespace mongo
