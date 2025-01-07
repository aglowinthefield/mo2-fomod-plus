#ifndef CONDITIONTESTER_H
#define CONDITIONTESTER_H

#include "DialogStateManager.h"
#include "../xml/ModuleConfiguration.h"


class ConditionTester {
public:
  explicit ConditionTester(DialogStateManager* stateManager) : mStateManager(stateManager) {}
  bool isStepVisible(const InstallStep& step) const;

private:
  DialogStateManager* mStateManager;
  std::unordered_map<std::string, std::string> mFlags;
};



#endif //CONDITIONTESTER_H
