#include "expression_quote.h"

namespace mongo {
REGISTER_STABLE_EXPRESSION(quote, ExpressionQuote::parse);

boost::intrusive_ptr<Expression>
ExpressionQuote::parse(ExpressionContext *expCtx, BSONElement expr,
                       const VariablesParseState &vps) {}

} // namespace mongo
