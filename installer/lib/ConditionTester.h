#ifndef CONDITIONTESTER_H
#define CONDITIONTESTER_H

#include <imoinfo.h>

#include "FlagMap.h"
#include "../xml/ModuleConfiguration.h"

class CompositeDependency;

class ConditionTester {
public:
    explicit ConditionTester(MOBase::IOrganizer* organizer) : mOrganizer(organizer) {}

    bool testCompositeDependency(const std::shared_ptr<FlagMap> &flags, const CompositeDependency &compositeDependency) const;

    static bool testFlagDependency(const std::shared_ptr<FlagMap> &flags, const FlagDependency &flagDependency);

    [[nodiscard]] bool testFileDependency(const FileDependency& fileDependency) const;

    bool testGameDependency(const GameDependency& gameDependency) const;

private:
    MOBase::IOrganizer* mOrganizer;

    friend class FomodViewModel;

    [[nodiscard]] FileDependencyTypeEnum getFileDependencyStateForPlugin(const std::string& pluginName) const;

    PluginTypeEnum getPluginTypeDescriptorState(const std::shared_ptr<Plugin> &plugin, const std::shared_ptr<FlagMap> &flags) const;

    mutable std::unordered_map<std::string, FileDependencyTypeEnum> pluginStateCache;

};


#endif //CONDITIONTESTER_H