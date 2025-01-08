#include "DialogStateManager.h"

bool DialogStateManager::isStepVisible(const InstallStep &step) {
  return mConditionTester->isStepVisible(mFlags, step);
}

void DialogStateManager::setFlag(const std::string &flag, const std::string &value) {
  mFlags.setFlag(flag, value);
}

std::string DialogStateManager::getFlag(const std::string &flag) {
  return mFlags.getFlag(flag);
}
