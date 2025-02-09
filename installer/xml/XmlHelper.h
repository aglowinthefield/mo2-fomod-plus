#ifndef XMLHELPER_H
#define XMLHELPER_H

#include "ModuleConfiguration.h"

class XmlHelper {
public:
    static OrderTypeEnum getOrderType(const std::string& orderType)
    {
        if (orderType == "Explicit")
            return OrderTypeEnum::Explicit;
        if (orderType == "Ascending")
            return OrderTypeEnum::Ascending;
        if (orderType == "Descending")
            return OrderTypeEnum::Descending;
        return OrderTypeEnum::Explicit; // No sorting by default. F the spec! (jk)
    }
};

#endif //XMLHELPER_H