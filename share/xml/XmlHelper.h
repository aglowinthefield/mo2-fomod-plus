#pragma once

#include "ModuleConfiguration.h"

class XmlHelper {
public:
    static OrderTypeEnum getOrderType(const std::string& orderType, OrderTypeEnum defaultOrder = OrderTypeEnum::Explicit)
    {
        if (orderType == "Explicit")
            return OrderTypeEnum::Explicit;
        if (orderType == "Ascending")
            return OrderTypeEnum::Ascending;
        if (orderType == "Descending")
            return OrderTypeEnum::Descending;
        return defaultOrder; // Ascending for plugins, Explicit for groups
    }
};