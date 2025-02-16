#include "ConditionTester.h"

#include "ui/FomodViewModel.h"

#include <iplugingame.h>
#include <ipluginlist.h>

std::string setToString(const std::set<int>& set)
{
    std::string str;
    for (const auto& i : set) {
        str += std::to_string(i) + ", ";
    }
    return str;
}

std::string ConditionTester::getValueForFlag(const std::string& flagName, const StepRefList& steps,
    const int stepIndex)
{
    std::vector<Flag> allFlags;
    auto step = steps[stepIndex];

    while (step) {
        auto theseFlags = step->getFlagsForIndividualStep();
        allFlags.insert(allFlags.end(), theseFlags.begin(), theseFlags.end());
        step = step->getPrevStep();
    }

    // Find the first flag that matches the flagName
    const auto it = std::ranges::find_if(allFlags, [&flagName](const Flag& flag) {
            return flag.first == flagName;
    });
    return it != allFlags.end() ? it->second : "";

}

bool ConditionTester::isStepVisible(const int stepIndex, const StepRefList& steps) const
{
    const CompositeDependency& condition = steps[stepIndex]->getVisibilityConditions();

    // first things first: is the current step visible
    if (!testCompositeDependency(condition, steps, stepIndex)) {
        return false;
    }

    const auto flagDependencies = condition.flagDependencies;
    if (flagDependencies.empty()) {
        return true;
    }

    /*
     * For the given step N to be visible, each flag dependency it has must be set by a step that's visible
     */
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
    const auto anyVisible = std::ranges::any_of(stepsThatSetThisFlag, [this, &steps](const int index) {
        return isStepVisible(index, steps);
    });
    if (!anyVisible) {
        log.logMessage(DEBUG, "Step " + steps[stepIndex]->getName() + " has no dependent steps that are visible.");
        log.logMessage(DEBUG, "Steps that set this flag: " + setToString(stepsThatSetThisFlag));
    }
    return anyVisible;

}

bool ConditionTester::testCompositeDependency(
    const CompositeDependency& compositeDependency, const StepRefList& steps, const int stepIndex) const
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
        results.emplace_back(testFlagDependency(flagDependency, steps, stepIndex));
    }
    for (const auto& gameDependency : gameDependencies) {
        results.emplace_back(testGameDependency(gameDependency));
    }
    for (const auto& nestedDependency : nestedDependencies) {
        results.emplace_back(testCompositeDependency(nestedDependency, steps, stepIndex));
    }

    if (globalOperatorType == OperatorTypeEnum::AND) {
        return std::ranges::all_of(results, [](const bool result) { return result; });
    }
    return std::ranges::any_of(results, [](const bool result) { return result; });
}


bool ConditionTester::testFlagDependency(const FlagDependency& flagDependency, const StepRefList& steps,
    const int stepIndex) const
{
    const auto flagValue = getValueForFlag(flagDependency.flag, steps, stepIndex);
    return flagValue == flagDependency.value;
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

PluginTypeEnum ConditionTester::getPluginTypeDescriptorState(PluginRef plugin, const StepRefList& steps) const
{
    // NOTE: A plugin's ConditionFlags aren't the same thing as a step visibility one.
    // A plugin's ConditionFlags are toggled based on the selection state of the plugin
    // We only evaluate the typeDescriptor here.

    // We will return the 'winning' type or the default. If multiple conditions are met,
    // ...well, I'm not sure.
    // ReSharper disable once CppTooWideScopeInitStatement
    const auto& typeDescriptor = plugin->getTypeDescriptor();
    const auto& dependencyType = typeDescriptor.dependencyType;

    for (const auto& pattern : dependencyType.patterns.patterns) {
        if (testCompositeDependency(pattern.dependencies, steps, plugin->getStepIndex())) {
            return pattern.type;
        }
    }

    // Sometimes authors do this.
    if (typeDescriptor.type != PluginTypeEnum::Optional) {
        return plugin->getTypeDescriptor().type;
    }
    if (dependencyType.defaultType.has_value()) {
        return plugin->getTypeDescriptor().dependencyType.defaultType.value();
    }
    return PluginTypeEnum::Optional;
}