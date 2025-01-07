#include "DialogStateManager.h"

void DialogStateManager::setFlag(const std::string &flag, const std::string &value) {
  mFlags[flag] = value;
}

std::string DialogStateManager::getFlag(const std::string &flag) const {
  const auto it = mFlags.find(flag);
  return it == mFlags.end() ? "" : it->second;
}
