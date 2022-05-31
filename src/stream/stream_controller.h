#pragma once
#include <vector>

#include "mongo/db/exec/document_value/document.h"
#include "mongo/db/pipeline/document_source.h"
#include "mongo/util/intrusive_counter.h"

namespace mongo {

class StreamController : public RefCountable {
public:

    static boost::intrusive_ptr<StreamController> create();
    StreamController() {};

    void setSource(boost::intrusive_ptr<DocumentSource> source) {
        _source = source;
    }

    DocumentSource::GetNextResult getNext();

private:

    void loadNextBatch();

    boost::intrusive_ptr<DocumentSource> _source = nullptr;

    std::vector<DocumentSource::GetNextResult> _buffer;

    Document _lastResult;
};
}