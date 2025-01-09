#ifndef CONDITIONTESTER_H
#define CONDITIONTESTER_H

#include <imoinfo.h>

#include "FlagMap.h"
#include "../xml/ModuleConfiguration.h"


class ConditionTester {
public:
  explicit ConditionTester(MOBase::IOrganizer* organizer) : mOrganizer(organizer) {}

  bool testCompositeDependency(const FlagMap &flags, const CompositeDependency &compositeDependency) const;

  bool isStepVisible(const FlagMap &flags, const InstallStep &step) const;

  static bool testFlagDependency(FlagMap flags, const FlagDependency &flagDependency);
  [[nodiscard]] bool testFileDependency(const FileDependency &fileDependency) const;

private:
  MOBase::IOrganizer* mOrganizer;

  friend class FomodViewModel;

  [[nodiscard]] FileDependencyTypeEnum getFileDependencyStateForPlugin(const std::string& pluginName) const;

  PluginTypeEnum getPluginTypeDescriptorState(const Plugin &plugin, const FlagMap &flags) const;
  mutable std::unordered_map<std::string, FileDependencyTypeEnum> pluginStateCache;

};



#endif //CONDITIONTESTER_H
