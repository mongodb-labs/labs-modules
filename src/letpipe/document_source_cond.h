#pragma once
#include <boost/intrusive_ptr.hpp>
#include <boost/optional.hpp>
#include <map>
#include <memory>
#include <tuple>
#include <vector>

#include "mongo/db/pipeline/document_source.h"
#include "mongo/db/pipeline/expression.h"
#include "mongo/db/pipeline/expression_context.h"
#include "mongo/db/pipeline/expression_visitor.h"
#include "mongo/db/pipeline/lite_parsed_document_source.h"
#include "mongo/db/pipeline/lite_parsed_pipeline.h"
#include "mongo/db/pipeline/pipeline.h"
#include "mongo/db/pipeline/variable_validation.h"
#include "mongo/db/pipeline/variables.h"

#include "document_source_expand.h"

namespace mongo {

class DocumentSourceCond final : public DocumentSource {
public:
  static boost::intrusive_ptr<DocumentSource>
  createFromBson(BSONElement elem,
                 const boost::intrusive_ptr<ExpressionContext> &pExpCtx);

private:
  DocumentSourceCond() = default;
};

} // namespace mongo
