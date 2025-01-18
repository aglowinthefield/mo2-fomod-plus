#ifndef XMLHELPER_H
#define XMLHELPER_H

#include "ModuleConfiguration.h"

class XmlHelper {
  public:
    static OrderTypeEnum getOrderType(const std::string &orderType) {
      if (orderType == "Explicit") return OrderTypeEnum::Explicit;
      if (orderType == "Ascending") return OrderTypeEnum::Ascending;
      if (orderType == "Descending") return OrderTypeEnum::Descending;
      return OrderTypeEnum::Ascending; // This is the default in the spec for some reason.
    }
};

#endif //XMLHELPER_H
