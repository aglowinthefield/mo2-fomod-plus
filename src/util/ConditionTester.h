#ifndef CONDITIONTESTER_H
#define CONDITIONTESTER_H

#include <imoinfo.h>

#include "FlagMap.h"
#include "../xml/ModuleConfiguration.h"


class ConditionTester {
public:
  bool isStepVisible(FlagMap &flags, const InstallStep &step) const;

  static bool testFlagDependency(FlagMap &flags, const FlagDependency &flagDependency);
  bool testFileDependency(const FileDependency &fileDependency) const;

private:
  explicit ConditionTester(MOBase::IOrganizer* organizer) : mOrganizer(organizer) {}
  MOBase::IOrganizer* mOrganizer;

  friend class DialogStateManager;

  FileDependencyTypeEnum getFileDependencyStateForPlugin(const std::string& pluginName) const;
};



#endif //CONDITIONTESTER_H
