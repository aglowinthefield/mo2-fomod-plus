#include "ConditionTester.h"

#include "ui/FomodViewModel.h"

#include <iplugingame.h>
#include <ipluginlist.h>

bool ConditionTester::isStepVisible(const std::shared_ptr<FlagMap>& flags,
    const CompositeDependency& compositeDependency,
    const int stepIndex,
    const std::vector<std::shared_ptr<StepViewModel>>& steps) const
{
    // first things first: is it visible?
    if (!testCompositeDependency(flags, compositeDependency)) {
        return false;
    }

    const auto flagDependencies = compositeDependency.flagDependencies;
    if (flagDependencies.empty()) {
        return true;
    }

    std::set<int> stepsThatSetThisFlag;

    for (const auto& flagDependency : flagDependencies) {
        // for this flag, find the plugins that set it
        for (int i = stepIndex - 1; i >= 0; --i) {
            for (const auto& group : steps[i]->getGroups()) {
                for (const auto& plugin : group->getPlugins()) {
                    if (std::ranges::any_of(plugin->getPlugin()->conditionFlags.flags,
                        [&flagDependency](const ConditionFlag& flag) {
                            return flag.name == flagDependency.flag && flag.value == flagDependency.value;
                        })) {
                            stepsThatSetThisFlag.insert(i);
                        }
                }
            }
        }
    }
    return std::ranges::any_of(stepsThatSetThisFlag, [this, &steps, &flags](const int stepIndex) {
        return isStepVisible(flags, steps[stepIndex]->getVisibilityConditions(), stepIndex, steps);
    });

}

bool ConditionTester::testCompositeDependency(const std::shared_ptr<FlagMap>& flags,
    const CompositeDependency& compositeDependency) const
{
    // if (compositeDependency.totalDependencies == 0) {
    //     return true;
    // }
    //
    // Log the flags
    // flags->forEach([this](const std::string& flag, const std::string& value) {
    //     log.logMessage(DEBUG, "Flag: " + flag + ", Value: " + value);
    // });

    const auto fileDependencies   = compositeDependency.fileDependencies;
    const auto flagDependencies   = compositeDependency.flagDependencies;
    const auto gameDependencies   = compositeDependency.gameDependencies;
    const auto nestedDependencies = compositeDependency.nestedDependencies;
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
    for (const auto& nestedDependency : nestedDependencies) {
        results.emplace_back(testCompositeDependency(flags, nestedDependency));
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
    // ReSharper disable once CppTooWideScopeInitStatement
    const auto& dependencyType = plugin->typeDescriptor.dependencyType;
    for (const auto& pattern : dependencyType.patterns.patterns) {
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