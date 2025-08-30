#include "ConditionTester.h"

#include "ui/FomodViewModel.h"

#include <iplugingame.h>
#include <ipluginlist.h>

std::string setToString(const std::set<int> &set)
{
    std::string str;
    for (const auto& i : set) {
        str += std::to_string(i) + ", ";
    }
    return str;
}

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
    const auto anyVisible = std::ranges::any_of(stepsThatSetThisFlag, [this, &steps, &flags](const int index) {
        return isStepVisible(flags, steps[index]->getVisibilityConditions(), index, steps);
    });
    if (!anyVisible) {
        log.logMessage(DEBUG, "Step " + steps[stepIndex]->getName() + " has no dependent steps that are visible.");
        log.logMessage(DEBUG, "Steps that set this flag: " + setToString(stepsThatSetThisFlag));
    }
    return anyVisible;

}

bool ConditionTester::testCompositeDependency(const std::shared_ptr<FlagMap>& flags,
    const CompositeDependency& compositeDependency) const
{
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
    return std::ranges::any_of(results, [](const bool result) { return result; });
}


bool ConditionTester::testFlagDependency(const std::shared_ptr<FlagMap>& flags, const FlagDependency& flagDependency)
{
    // Every instance of this flag being set in the map.
    const auto flagList = flags->getFlagsByKey(flagDependency.flag);

    // Find the first instance of this flag being set (in the order specified by getFlagsByKey)
    if (flagList.empty()) {
        // If the dependency value is an empty string, it means this flag should be unset.
        // So if we don't have any value for this flag, the result is true.
        return flagDependency.value.empty();
    }

    return flagList.front().second == flagDependency.value;
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
    log.logMessage(DEBUG, "Comparing condition version " + gameDependency.version + " against " + gameVersion);
    if ( gameDependency.version <= gameVersion) {
        log.logMessage(DEBUG, "Version matches!");
    }
    return gameDependency.version <= gameVersion;
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