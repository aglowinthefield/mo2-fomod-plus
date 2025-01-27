#include "ConditionTester.h"

#include <iplugingame.h>
#include <ipluginlist.h>

bool ConditionTester::testCompositeDependency(const std::shared_ptr<FlagMap>& flags,
    const CompositeDependency& compositeDependency) const
{
    if (compositeDependency.totalDependencies == 0) {
        return true;
    }

    const auto fileDependencies   = compositeDependency.fileDependencies;
    const auto flagDependencies   = compositeDependency.flagDependencies;
    const auto gameDependencies   = compositeDependency.gameDependencies;
    const auto globalOperatorType = compositeDependency.operatorType;

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
    for (const auto& gameDependency : gameDependencies) {
        results.emplace_back(testGameDependency(gameDependency));
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
bool ConditionTester::isStepVisible(const std::shared_ptr<FlagMap>& flags,
    const std::shared_ptr<InstallStep>& step) const
{
    return testCompositeDependency(flags, step->visible);
}

bool ConditionTester::testFlagDependency(const std::shared_ptr<FlagMap>& flags, const FlagDependency& flagDependency)
{
    return flags->getFlag(flagDependency.flag) == flagDependency.value;
}

bool ConditionTester::testFileDependency(const FileDependency& fileDependency) const
{
    const std::string& pluginName = fileDependency.file;
    const auto pluginState        = getFileDependencyStateForPlugin(pluginName);
    return pluginState == fileDependency.state;
}

bool ConditionTester::testGameDependency(const GameDependency& gameDependency) const
{
    const auto gameVersion = mOrganizer->managedGame()->gameVersion().toStdString();
    return gameVersion == gameDependency.version;
}

FileDependencyTypeEnum ConditionTester::getFileDependencyStateForPlugin(const std::string& pluginName) const
{
    if (const auto it = pluginStateCache.find(pluginName); it != pluginStateCache.end()) {
        return it->second;
    }

    const QFlags<MOBase::IPluginList::PluginState> pluginState = mOrganizer->pluginList()->state(
        QString::fromStdString(pluginName));

    FileDependencyTypeEnum state;

    if (pluginState == MOBase::IPluginList::STATE_MISSING) {
        state = FileDependencyTypeEnum::Missing;
    } else if (pluginState == MOBase::IPluginList::STATE_INACTIVE) {
        state = FileDependencyTypeEnum::Inactive;
    } else if (pluginState == MOBase::IPluginList::STATE_ACTIVE) {
        state = FileDependencyTypeEnum::Active;
    } else {
        state = FileDependencyTypeEnum::UNKNOWN_STATE;
    }

    pluginStateCache[pluginName] = state;
    return state;
}

PluginTypeEnum ConditionTester::getPluginTypeDescriptorState(const std::shared_ptr<Plugin>& plugin,
    const std::shared_ptr<FlagMap>& flags) const
{
    // NOTE: A plugin's ConditionFlags aren't the same thing as a step visibility one.
    // A plugin's ConditionFlags are toggled based on the selection state of the plugin
    // We only evaluate the typeDescriptor here.

    // We will return the 'winning' type or the default. If multiple conditions are met,
    // ...well, I'm not sure.
    for (const auto& dependencyType = plugin->typeDescriptor.dependencyType; const auto& pattern : dependencyType.
         patterns.patterns) {
        if (testCompositeDependency(flags, pattern.dependencies)) {
            return pattern.type;
        }
    }

    // Sometimes authors do this.
    if (plugin->typeDescriptor.type != PluginTypeEnum::Optional) {
        return plugin->typeDescriptor.type;
    }
    if (plugin->typeDescriptor.dependencyType.defaultType.has_value()) {
        return plugin->typeDescriptor.dependencyType.defaultType.value();
    }
    return PluginTypeEnum::Optional;
}