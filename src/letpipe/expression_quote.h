#pragma once

#include <boost/intrusive_ptr.hpp>
#include <boost/optional.hpp>

#include <vector>

#include "mongo/db/pipeline/dependencies.h"
#include "mongo/db/pipeline/expression.h"
#include "mongo/db/pipeline/expression_context.h"
#include "mongo/db/pipeline/expression_visitor.h"
#include "mongo/db/pipeline/lite_parsed_pipeline.h"
#include "mongo/db/pipeline/pipeline.h"
#include "mongo/db/pipeline/variable_validation.h"
#include "mongo/db/pipeline/variables.h"

namespace mongo {
class ExpressionQuote final : public Expression {
public:
  static boost::intrusive_ptr<Expression> parse(ExpressionContext *expCtx,
                                                BSONElement expr,
                                                const VariablesParseState &vps);
  Value evaluate(const Document &root, Variables *variables) const final;
  boost::intrusive_ptr<Expression> optimize() final { return this; };
  Value serialize(bool explain) const final {
    return Value(Document{{"$quote", _quoted}});
  };

  void acceptVisitor(ExpressionConstVisitor *v) const final {
    return v->visit(this);
  }

  void acceptVisitor(ExpressionMutableVisitor *v) final {
    return v->visit(this);
  }

protected:
  void _doAddDependencies(DepsTracker *deps) const final{};

private:
  explicit ExpressionQuote(
      ExpressionContext *expCtx, Value quoted,
      std::vector<boost::intrusive_ptr<Expression>> children)
      : Expression(expCtx, std::move(children)), _quoted(std::move(quoted)) {}

  Value _quoted;
};
} // namespace mongo
