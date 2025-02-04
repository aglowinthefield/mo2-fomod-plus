#ifndef DIALOGSTATEMANAGER_H
#define DIALOGSTATEMANAGER_H
#include <imoinfo.h>
#include <string>

#include "../lib/FlagMap.h"
#include "../lib/ConditionTester.h"
#include "lib/FileInstaller.h"
#include "xml/FomodInfoFile.h"

template <typename T>
using shared_ptr_list = std::vector<std::shared_ptr<T> >;

/*
--------------------------------------------------------------------------------
                                Plugins
--------------------------------------------------------------------------------
*/
class PluginViewModel {
public:
    PluginViewModel(const std::shared_ptr<Plugin>& plugin_, const bool selected, bool)
        : plugin(plugin_), selected(selected), enabled(true) {}

    void setSelected(const bool selected) { this->selected = selected; }
    void setEnabled(const bool enabled) { this->enabled = enabled; }
    [[nodiscard]] std::string getName() const { return plugin->name; }
    [[nodiscard]] std::string getDescription() const { return plugin->description; }
    [[nodiscard]] std::string getImagePath() const { return plugin->image.path; }
    [[nodiscard]] bool isSelected() const { return selected; }
    [[nodiscard]] bool isEnabled() const { return enabled; }

    friend class FomodViewModel;
    friend class FileInstaller;

protected:
    [[nodiscard]] std::shared_ptr<Plugin> getPlugin() const { return plugin; }
    std::shared_ptr<Plugin> plugin;

private:
    bool selected;
    bool enabled;
};

/*
--------------------------------------------------------------------------------
                                Groups
--------------------------------------------------------------------------------
*/
class GroupViewModel {
public:
    GroupViewModel(const std::shared_ptr<Group>& group_, const shared_ptr_list<PluginViewModel>& plugins)
        : plugins(plugins), group(group_) {}

    [[nodiscard]] std::string getName() const { return group->name; }
    [[nodiscard]] GroupTypeEnum getType() const { return group->type; }
    [[nodiscard]] shared_ptr_list<PluginViewModel> getPlugins() const { return plugins; }

    friend class FomodViewModel;
    friend class FileInstaller;

protected:
    shared_ptr_list<PluginViewModel> plugins;
    std::shared_ptr<Group> group;
};

/*
--------------------------------------------------------------------------------
                                Steps
--------------------------------------------------------------------------------
*/
class StepViewModel {
public:
    StepViewModel(const std::shared_ptr<InstallStep>& installStep_, const shared_ptr_list<GroupViewModel>& groups)
        : installStep(installStep_), groups(groups) {}

    [[nodiscard]] std::string getName() const { return installStep->name; }
    [[nodiscard]] const shared_ptr_list<GroupViewModel>& getGroups() const { return groups; }

    friend class FomodViewModel;
    friend class FileInstaller;

protected:
    std::shared_ptr<InstallStep> installStep;
    // bool isVisible = true;
    shared_ptr_list<GroupViewModel> groups;
};

/*
--------------------------------------------------------------------------------
                                Info
--------------------------------------------------------------------------------
*/
class InfoViewModel {
public:
    explicit InfoViewModel(const std::unique_ptr<FomodInfoFile>& infoFile)
    {
        if (infoFile) {
            // Copy the necessary members from FomodInfoFile to InfoViewModel
            mName    = infoFile->getName();
            mVersion = infoFile->getVersion();
            mAuthor  = infoFile->getAuthor();
            mWebsite = infoFile->getWebsite();
        }
    }

    // Accessor methods
    [[nodiscard]] std::string getName() const { return mName; }
    [[nodiscard]] std::string getVersion() const { return mVersion; }
    [[nodiscard]] std::string getAuthor() const { return mAuthor; }
    [[nodiscard]] std::string getWebsite() const { return mWebsite; }

private:
    std::string mName;
    std::string mVersion;
    std::string mAuthor;
    std::string mWebsite;
};

/*
--------------------------------------------------------------------------------
                               View Model
--------------------------------------------------------------------------------
*/
enum class NEXT_OP { NEXT, INSTALL };

class FomodViewModel {
public:
    FomodViewModel(
        MOBase::IOrganizer* organizer,
        std::unique_ptr<ModuleConfiguration> fomodFile,
        std::unique_ptr<FomodInfoFile> infoFile);

    static std::shared_ptr<FomodViewModel> create(
        MOBase::IOrganizer* organizer,
        std::unique_ptr<ModuleConfiguration> fomodFile,
        std::unique_ptr<FomodInfoFile> infoFile);

    void forEachGroup(
        const std::function<void(const std::shared_ptr<GroupViewModel>&)> &callback)
    const;

    void forEachPlugin(
        const std::function<void(const std::shared_ptr<GroupViewModel>&, const std::shared_ptr<PluginViewModel>&)>&
        callback)
    const;

    void forEachFuturePlugin(
        const std::function<void(const std::shared_ptr<GroupViewModel>&, const std::shared_ptr<PluginViewModel>&)>&
        callback)
    const;

    [[nodiscard]] const std::shared_ptr<PluginViewModel>& getFirstPluginForActiveStep() const;

    // Steps
    [[nodiscard]] shared_ptr_list<StepViewModel> getSteps() const { return mSteps; }
    [[nodiscard]] const std::shared_ptr<StepViewModel>& getActiveStep() const { return mActiveStep; }
    [[nodiscard]] int getCurrentStepIndex() const { return mCurrentStepIndex; }
    [[deprecated]] void setCurrentStepIndex(const int index) { mCurrentStepIndex = index; }

    void updateVisibleSteps() const;

    void preinstall(const std::shared_ptr<MOBase::IFileTree>& tree, const QString& fomodPath);

    std::shared_ptr<FileInstaller> getFileInstaller() { return mFileInstaller; }

    std::string getDisplayImage() const;

    // Plugins
    [[nodiscard]] std::shared_ptr<PluginViewModel> getActivePlugin() const { return mActivePlugin; }

    // Info
    [[nodiscard]] std::shared_ptr<InfoViewModel> getInfoViewModel() const { return mInfoViewModel; }

    // Interactions
    void stepBack();

    void stepForward();

    bool isLastVisibleStep() const;

    void togglePlugin(const std::shared_ptr<GroupViewModel>&, const std::shared_ptr<PluginViewModel>& plugin,
        bool selected) const;

    void setActivePlugin(const std::shared_ptr<PluginViewModel>& plugin) const { mActivePlugin = plugin; }

private:
    MOBase::IOrganizer* mOrganizer = nullptr;
    std::unique_ptr<ModuleConfiguration> mFomodFile;
    std::unique_ptr<FomodInfoFile> mInfoFile;
    std::shared_ptr<FlagMap> mFlags {nullptr};
    ConditionTester mConditionTester;
    std::shared_ptr<InfoViewModel> mInfoViewModel;
    std::vector<std::shared_ptr<StepViewModel> > mSteps;
    mutable std::shared_ptr<PluginViewModel> mActivePlugin{ nullptr };
    mutable std::shared_ptr<StepViewModel> mActiveStep{ nullptr };
    mutable std::vector<int> mVisibleStepIndices;
    std::shared_ptr<FileInstaller> mFileInstaller{ nullptr };
    bool mInitialized{ false };

    void createStepViewModels();

    void setFlagForPluginState(const std::shared_ptr<PluginViewModel>& plugin, bool selected) const;

    void createNonePluginForGroup(const std::shared_ptr<GroupViewModel>& group) const;

    void processPlugin(const std::shared_ptr<GroupViewModel>& groupViewModel,
        const std::shared_ptr<PluginViewModel>& pluginViewModel) const;

    void enforceRadioGroupConstraints(const std::shared_ptr<GroupViewModel>& groupViewModel) const;

    void enforceSelectAllConstraint(const std::shared_ptr<GroupViewModel>& groupViewModel) const;

    void enforceSelectAtLeastOneConstraint(const std::shared_ptr<GroupViewModel>& groupViewModel) const;

    void enforceGroupConstraints() const;

    void processPluginConditions() const;


    // Indices
    int mCurrentStepIndex{ 0 };
};


#endif //DIALOGSTATEMANAGER_H