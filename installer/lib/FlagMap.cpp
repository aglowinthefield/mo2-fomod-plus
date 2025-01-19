#include "FlagMap.h"

std::string FlagMap::getFlag(const std::string& flag)
{
    const auto it = find(flag);
    return it == end() ? "" : it->second;
}

void FlagMap::setFlag(const std::string& flag, const std::string& value)
{
    (*this)[flag] = value;
}