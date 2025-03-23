#pragma once

#include <imoinfo.h>
#include <string>

#include "../lib/ConditionTester.h"
#include "../lib/FlagMap.h"
#include "lib/FileInstaller.h"
#include "lib/ViewModels.h"
#include "xml/FomodInfoFile.h"

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
        const std::function<void(const std::shared_ptr<GroupViewModel>&)>& callback)
    const;

    void forEachPlugin(
        const std::function<void(const std::shared_ptr<GroupViewModel>&, const std::shared_ptr<PluginViewModel>&)>&
        callback)
    const;

    void forEachFuturePlugin(
        int fromStepIndex, const std::function<void(const std::shared_ptr<GroupViewModel>&, const std::shared_ptr<
            PluginViewModel>&)> &callback)
    const;

    void selectFromJson(nlohmann::json json) const;

    [[nodiscard]] const std::shared_ptr<PluginViewModel>& getFirstPluginForActiveStep() const;

    // Steps
    [[nodiscard]] shared_ptr_list<StepViewModel> getSteps() const { return mSteps; }
    [[nodiscard]] const std::shared_ptr<StepViewModel>& getActiveStep() const { return mActiveStep; }
    [[nodiscard]] int getCurrentStepIndex() const { return mCurrentStepIndex; }
    [[deprecated]] void setCurrentStepIndex(const int index) { mCurrentStepIndex = index; }

    void updateVisibleSteps() const;

    void rebuildConditionFlagsForStep(int stepIndex) const;

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

    bool isFirstVisibleStep() const;

    void togglePlugin(const std::shared_ptr<GroupViewModel>&, const std::shared_ptr<PluginViewModel>& plugin,
        bool selected) const;

    void setActivePlugin(const std::shared_ptr<PluginViewModel>& plugin) const { mActivePlugin = plugin; }

private:
    Logger& log = Logger::getInstance();
    MOBase::IOrganizer* mOrganizer = nullptr;
    std::unique_ptr<ModuleConfiguration> mFomodFile;
    std::unique_ptr<FomodInfoFile> mInfoFile;
    std::shared_ptr<FlagMap> mFlags{ nullptr };
    ConditionTester mConditionTester;
    std::shared_ptr<InfoViewModel> mInfoViewModel;
    std::vector<std::shared_ptr<StepViewModel> > mSteps;
    mutable std::shared_ptr<PluginViewModel> mActivePlugin{ nullptr };
    mutable std::shared_ptr<StepViewModel> mActiveStep{ nullptr };
    mutable std::vector<int> mVisibleStepIndices;
    std::shared_ptr<FileInstaller> mFileInstaller{ nullptr };
    bool mInitialized{ false };

    void createStepViewModels();

    void setFlagForPluginState(const std::shared_ptr<PluginViewModel> &plugin) const;

    static void createNonePluginForGroup(const std::shared_ptr<GroupViewModel>& group) ;

    void processPlugin(const std::shared_ptr<GroupViewModel>& group,
        const std::shared_ptr<PluginViewModel>& plugin) const;

    void enforceRadioGroupConstraints(const std::shared_ptr<GroupViewModel>& group) const;

    void enforceSelectAllConstraint(const std::shared_ptr<GroupViewModel>& groupViewModel) const;

    void enforceSelectAtLeastOneConstraint(const std::shared_ptr<GroupViewModel>& group) const;

    void enforceGroupConstraints() const;

    void processPluginConditions(int fromStepIndex) const;

    // Indices
    int mCurrentStepIndex{ 0 };

    void logMessage(const LogLevel level, const std::string& message) const
    {
        log.logMessage(level, "[VIEWMODEL] " + message);
    }

    std::string toString() const;
};
