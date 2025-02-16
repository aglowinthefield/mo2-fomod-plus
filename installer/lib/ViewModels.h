#ifndef VIEWMODELS_H
#define VIEWMODELS_H
#include "xml/ModuleConfiguration.h"


template <typename T>
using shared_ptr_list = std::vector<std::shared_ptr<T> >;

using Flag     = std::pair<std::string, std::string>;
using FlagList = std::vector<Flag>;


/*
--------------------------------------------------------------------------------
                                Plugins
--------------------------------------------------------------------------------
*/
class PluginViewModel {
public:
    PluginViewModel(const std::shared_ptr<Plugin>& plugin_, const bool selected, bool, const int index)
        : ownIndex(index), selected(selected), enabled(true), plugin(plugin_) {}

    void setSelected(const bool selected) { this->selected = selected; }
    void setEnabled(const bool enabled) { this->enabled = enabled; }
    [[nodiscard]] std::string getName() const { return plugin->name; }
    [[nodiscard]] std::string getDescription() const { return plugin->description; }
    [[nodiscard]] std::string getImagePath() const { return plugin->image.path; }
    [[nodiscard]] bool isSelected() const { return selected; }
    [[nodiscard]] bool isEnabled() const { return enabled; }
    [[nodiscard]] std::vector<ConditionFlag> getConditionFlags() const { return plugin->conditionFlags.flags; }
    [[nodiscard]] int getOwnIndex() const { return ownIndex; }
    [[nodiscard]] int getStepIndex() const { return stepIndex; }
    [[nodiscard]] int getGroupIndex() const { return groupIndex; }
    [[nodiscard]] PluginTypeEnum getCurrentPluginType() const { return currentPluginType; }
    [[nodiscard]] TypeDescriptor getTypeDescriptor() const { return plugin->typeDescriptor; }
    void setCurrentPluginType(const PluginTypeEnum type) { currentPluginType = type; }

    void setGroupIndex(const int groupIndex) { this->groupIndex = groupIndex; }
    void setStepIndex(const int stepIndex) { this->stepIndex = stepIndex; }

    friend class FomodViewModel;
    friend class FileInstaller;
    friend class ConditionTester;

protected:
    [[nodiscard]] std::shared_ptr<Plugin> getPlugin() const { return plugin; }

private:
    int ownIndex;
    int groupIndex{ -1 };
    int stepIndex{ -1 };
    bool selected;
    bool enabled;
    PluginTypeEnum currentPluginType = PluginTypeEnum::UNKNOWN;
    std::shared_ptr<Plugin> plugin;
};

/*
--------------------------------------------------------------------------------
                                Groups
--------------------------------------------------------------------------------
*/
class GroupViewModel {
public:
    GroupViewModel(const std::shared_ptr<Group>& group_, const shared_ptr_list<PluginViewModel>& plugins,
        const int index, const int stepIndex)
        : plugins(plugins), group(group_), ownIndex(index), stepIndex(stepIndex) {}

    void addPlugin(const std::shared_ptr<PluginViewModel>& plugin) { plugins.emplace_back(plugin); }

    [[nodiscard]] std::string getName() const { return group->name; }
    [[nodiscard]] GroupTypeEnum getType() const { return group->type; }
    [[nodiscard]] shared_ptr_list<PluginViewModel> getPlugins() const { return plugins; }
    [[nodiscard]] int getOwnIndex() const { return ownIndex; }
    [[nodiscard]] int getStepIndex() const { return stepIndex; }

private:
    shared_ptr_list<PluginViewModel> plugins;
    std::shared_ptr<Group> group;
    int ownIndex;
    int stepIndex;
};

/*
--------------------------------------------------------------------------------
                                Steps
--------------------------------------------------------------------------------
*/
class StepViewModel {
public:
    StepViewModel(const std::shared_ptr<InstallStep>& installStep_, const shared_ptr_list<GroupViewModel>& groups,
        const int index)
        : installStep(installStep_), groups(groups), ownIndex(index) {}

    [[nodiscard]] CompositeDependency& getVisibilityConditions() const { return installStep->visible; }
    [[nodiscard]] std::string getName() const { return installStep->name; }
    [[nodiscard]] const shared_ptr_list<GroupViewModel>& getGroups() const { return groups; }
    [[nodiscard]] int getOwnIndex() const { return ownIndex; }
    [[nodiscard]] bool getHasVisited() const { return visited; }
    [[nodiscard]] bool isVisible() const { return visible; }

    [[nodiscard]] std::shared_ptr<StepViewModel> getPrevStep() const { return prevStep; }
    [[nodiscard]] std::shared_ptr<StepViewModel> getNextStep() const { return nextStep; }

    [[nodiscard]] std::vector<Flag> getFlagsForIndividualStep() const
    {
        std::vector<Flag> flags;
        for (const auto& group : groups) {
            for (const auto& plugin : group->getPlugins()) {
                for (const auto& flag : plugin->getConditionFlags()) {
                    if (plugin->isSelected()) {
                        flags.emplace_back(flag.name, flag.value);
                    }
                }
            }
        }
        return flags;
    }

    void setVisited(const bool visited) { this->visited = visited; }
    void setVisible(const bool visible) { this->visible = visible; }
    void setPrevStep(const std::shared_ptr<StepViewModel>& prevStep) { this->prevStep = prevStep; }
    void setNextStep(const std::shared_ptr<StepViewModel>& nextStep) { this->nextStep = nextStep; }

private:
    bool visited{ false };
    std::shared_ptr<InstallStep> installStep;
    shared_ptr_list<GroupViewModel> groups;

    std::shared_ptr<StepViewModel> nextStep{ nullptr };
    std::shared_ptr<StepViewModel> prevStep{ nullptr };
    bool visible{false};
    int ownIndex;
};


/*
--------------------------------------------------------------------------------
                            Outbound Types
--------------------------------------------------------------------------------
*/
using GroupRef  = const std::shared_ptr<GroupViewModel>&;
using PluginRef = const std::shared_ptr<PluginViewModel>&;
using StepRef   = const std::shared_ptr<StepViewModel>&;

using StepRefList = shared_ptr_list<StepViewModel>;


#endif //VIEWMODELS_H