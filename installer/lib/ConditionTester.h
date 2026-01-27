#pragma once

#include <imoinfo.h>

#include "FlagMap.h"
#include "Logger.h"
#include "xml/ModuleConfiguration.h"

class CompositeDependency;
class StepViewModel;

class ConditionTester {
public:
    explicit ConditionTester(MOBase::IOrganizer* organizer) : mOrganizer(organizer) {}

    bool isStepVisible(const std::shared_ptr<FlagMap>& flags, const CompositeDependency& compositeDependency,
        int stepIndex,
        const std::vector<std::shared_ptr<StepViewModel>>& steps) const;

    bool testCompositeDependency(const std::shared_ptr<FlagMap>& flags,
        const CompositeDependency& compositeDependency) const;

    static bool testFlagDependency(const std::shared_ptr<FlagMap>& flags, const FlagDependency& flagDependency);

    [[nodiscard]] bool testFileDependency(const FileDependency& fileDependency) const;

    bool testGameDependency(const GameDependency& gameDependency) const;

private:
    Logger& log = Logger::getInstance();
    MOBase::IOrganizer* mOrganizer;

    friend class FomodViewModel;

    [[nodiscard]] FileDependencyTypeEnum getFileDependencyStateForPlugin(const std::string& pluginName) const;

    PluginTypeEnum getPluginTypeDescriptorState(const std::shared_ptr<Plugin>& plugin,
        const std::shared_ptr<FlagMap>& flags) const;

    mutable std::unordered_map<std::string, FileDependencyTypeEnum> pluginStateCache;

};
