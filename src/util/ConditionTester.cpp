#include "ConditionTester.h"
#include <ipluginlist.h>

bool ConditionTester::isStepVisible(FlagMap &flags, const InstallStep& step) const {
  const auto fileDependencies = step.visible.dependencies.fileDependencies;
  const auto flagDependencies = step.visible.dependencies.flagDependencies;
  const auto globalOperatorType = step.visible.dependencies.operatorType;

  if (fileDependencies.empty() && flagDependencies.empty()) {
    return true;
  }

  // For the globalOperatorType
  // Evaluate all conditions and store the results in a vector<bool>, then return based on operator.
  // These aren't expensive to calculate so rather than do some fancy logic to short-circuit, just calculate all of 'em.
  std::vector<bool> results;
  for (auto fileDependency : fileDependencies) {
    results.emplace_back(testFileDependency(fileDependency));
  }
  for (const auto& flagDependency : flagDependencies) {
    results.emplace_back(testFlagDependency(flags, flagDependency));
  }

  if (globalOperatorType == OperatorTypeEnum::AND) {
    return std::ranges::all_of(results, [](const bool result) { return result; });
  }
  if (globalOperatorType == OperatorTypeEnum::OR) {
    return std::ranges::any_of(results, [](const bool result) { return result; });
  }
  // Not sure why this would happen, but it's here for now.
  return true;
}

bool ConditionTester::testFlagDependency(FlagMap &flags, const FlagDependency& flagDependency) {
  return flags.getFlag(flagDependency.flag) == flagDependency.value;
}

bool ConditionTester::testFileDependency(const FileDependency& fileDependency) const {
  const std::string& pluginName = fileDependency.file;
  const auto pluginState = getFileDependencyStateForPlugin(pluginName);
  return pluginState == fileDependency.state;
}

FileDependencyTypeEnum ConditionTester::getFileDependencyStateForPlugin(const std::string &pluginName) const {
  const QFlags<MOBase::IPluginList::PluginState> pluginState = mOrganizer->pluginList()->state(QString::fromStdString(pluginName));

  if (pluginState & MOBase::IPluginList::STATE_MISSING) {
    return FileDependencyTypeEnum::Missing;
  }
  if (pluginState & MOBase::IPluginList::STATE_INACTIVE) {
    return FileDependencyTypeEnum::Inactive;
  }
  if (pluginState & MOBase::IPluginList::STATE_ACTIVE) {
    return FileDependencyTypeEnum::Active;
  }

  return FileDependencyTypeEnum::UNKNOWN_STATE;
}
