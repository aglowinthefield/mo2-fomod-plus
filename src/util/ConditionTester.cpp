#include "ConditionTester.h"
#include <ipluginlist.h>

bool ConditionTester::testCompositeDependency(const FlagMap& flags, const CompositeDependency& compositeDependency) const {
  const auto fileDependencies = compositeDependency.fileDependencies;
  const auto flagDependencies = compositeDependency.flagDependencies;
  const auto globalOperatorType = compositeDependency.operatorType;

  if (fileDependencies.empty() && flagDependencies.empty()) {
    return true;
  }
  // For the globalOperatorType
  // Evaluate all conditions and store the results in a vector<bool>, then return based on operator.
  // These aren't expensive to calculate so rather than do some fancy logic to short-circuit, just calculate all of 'em.
  std::vector<bool> results;
  for (const auto& fileDependency : fileDependencies) {
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


[[deprecated("Use testCompositeDependency() directly instead")]]
bool ConditionTester::isStepVisible(const FlagMap &flags, const InstallStep &step) const {
  return testCompositeDependency(flags, step.visible.dependencies);
}

bool ConditionTester::testFlagDependency(FlagMap flags, const FlagDependency& flagDependency) {
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

PluginTypeEnum ConditionTester::getPluginTypeDescriptorState(const Plugin &plugin, const FlagMap &flags) const {
  // NOTE: A plugin's ConditionFlags aren't the same thing as a step visibility one.
  // A plugin's ConditionFlags are toggled based on the selection state of the plugin
  // We only evaluate the typeDescriptor here.

  // We will return the 'winning' type or the default. If multiple conditions are met,
  // ...well, I'm not sure.

  // TODO: Cache the fileDependencyStates

  for (auto pattern : plugin.typeDescriptor.dependencyType.patterns.patterns) {
    if (testCompositeDependency(flags, pattern.dependencies)) {
      return pattern.type.name;
    }
  }
  return plugin.typeDescriptor.dependencyType.defaultType.name;
}
