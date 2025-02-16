#ifndef CONDITIONTESTER_H
#define CONDITIONTESTER_H

#include <imoinfo.h>

#include "FlagMap.h"
#include "Logger.h"
#include "../xml/ModuleConfiguration.h"

class CompositeDependency;
class StepViewModel;

class ConditionTester {
public:
    explicit ConditionTester(MOBase::IOrganizer* organizer) : mOrganizer(organizer) {}

    [[nodiscard]] static std::string getValueForFlag(const std::string& flagName, const StepRefList& steps,
        int stepIndex);

    [[nodiscard]] bool isStepVisible(int stepIndex, const StepRefList& steps) const;

    [[nodiscard]] bool testCompositeDependency(
        const CompositeDependency& compositeDependency, const StepRefList& steps, int stepIndex) const;

    [[nodiscard]] bool testFlagDependency(const FlagDependency& flagDependency, const StepRefList& steps,
        int stepIndex) const;

    [[nodiscard]] bool testFileDependency(const FileDependency& fileDependency) const;

    [[nodiscard]] bool testGameDependency(const GameDependency& gameDependency) const;

private:
    Logger& log = Logger::getInstance();
    MOBase::IOrganizer* mOrganizer;

    friend class FomodViewModel;

    [[nodiscard]] FileDependencyTypeEnum getFileDependencyStateForPlugin(const std::string& pluginName) const;

    PluginTypeEnum getPluginTypeDescriptorState(PluginRef plugin, const StepRefList &steps) const;

    mutable std::unordered_map<std::string, FileDependencyTypeEnum> pluginStateCache;

};


#endif //CONDITIONTESTER_H