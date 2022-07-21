#define MONGO_LOGV2_DEFAULT_COMPONENT ::mongo::logv2::LogComponent::kDefault

#include "mongo/logv2/log.h"

#include "mongo/platform/basic.h"

#include "document_source_cond.h"
#include "document_source_expand.h"

#include <string>

namespace mongo {
REGISTER_DOCUMENT_SOURCE(cond, LiteParsedDocumentSourceDefault::parse,
                         DocumentSourceCond::createFromBson,
                         AllowedWithApiStrict::kAlways);

boost::intrusive_ptr<DocumentSource> DocumentSourceCond::createFromBson(
    BSONElement elem, const boost::intrusive_ptr<ExpressionContext> &pExpCtx) {
  /*
  Syntax:
  { $cond:
    {
      branches: [
        {
          case: expr
          then: [pipeline]
        },
        ...
      ]
      default: [pipeline]
    }
  }
  Transform into
  { $expand
    { $switch:
      {
        branches: [
          {
            case: expr
            then: { $const: [pipeline] }
          }
        ],
        default: { $const: [pipeline] }
      }
    }
  }
  */
  LOGV2(99999999, "create cond", "elem"_attr = elem);

  bool hasDefault = false;
  BSONElement defaultPipe;
  BSONElement branches;

  for (auto &&arg : elem.embeddedObject()) {
    if (arg.fieldNameStringData() == "default") {
      hasDefault = true;
      defaultPipe = arg;
      LOGV2(99999999, "default pipe: ", "default"_attr = defaultPipe);
    } else if (arg.fieldNameStringData() == "branches") {
      branches = arg;
      LOGV2(99999999, "branches: ", "cases"_attr = branches);
    } else {
      uasserted(99999999, str::stream()
                              << "Unknown arg for $cond: " << arg.fieldName());
    }
  }

  uassert(99999999, "Missing branches in $cond", !branches.eoo());

  BSONObjBuilder switchBuilder;
  {
    BSONObjBuilder inner(switchBuilder.subobjStart("$switch"));
    BSONArrayBuilder branchesArr(inner.subarrayStart("branches"));
    int i = 0;
    for (auto &&caseObj : branches.embeddedObject()) {
      LOGV2(999999999, "case", "caseObj"_attr = caseObj);
      branchesArr.append(BSON("case" << caseObj["case"] << "then"
                                     << BSON("$const" << caseObj["then"])));
      i++;
    }
    branchesArr.done();
    if (hasDefault) {
      BSONObjBuilder defaultObj(inner.subobjStart("default"));
      BSONArrayBuilder constArr(defaultObj.subarrayStart("$const"));
      for (auto &&s : defaultPipe.Array()) {
        constArr.append(s);
      }
      constArr.done();
      defaultObj.doneFast();
    }
  }
  switchBuilder.doneFast();
  BSONObj switchObj = switchBuilder.obj();

  LOGV2(99999999, "constructed switchObj", "obj"_attr = switchObj);

  return DocumentSourceExpand::createFromBson(
      BSON("$expand" << switchObj).firstElement(), pExpCtx);
}

} // namespace mongo
