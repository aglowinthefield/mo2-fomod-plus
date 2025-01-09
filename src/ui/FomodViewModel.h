#ifndef DIALOGSTATEMANAGER_H
#define DIALOGSTATEMANAGER_H
#include <imoinfo.h>
#include <string>

#include "../util/FlagMap.h"
#include "../util/ConditionTester.h"
#include "xml/FomodInfoFile.h"

/*
--------------------------------------------------------------------------------
                                Plugins
--------------------------------------------------------------------------------
*/
class PluginViewModel {
public:
  PluginViewModel(const Plugin &plugin_, const bool selected, bool)
    : plugin(plugin_), selected(selected), enabled(true) {
  }

  void setSelected(const bool selected) { this->selected = selected; }
  void setEnabled(const bool enabled)   { this->enabled = enabled; }
  [[nodiscard]] std::string getName() const           { return plugin.description; }
  [[nodiscard]] std::string getImagePath() const      { return plugin.image.path; }
  [[nodiscard]] bool isSelected() const               { return selected; }
  [[nodiscard]] bool isEnabled() const                { return enabled; }
  [[nodiscard]] Plugin getPlugin() const              { return plugin; }

private:
  const Plugin plugin;
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
  GroupViewModel(const Group &group_, std::vector<PluginViewModel> plugins)
    : plugins(std::move(plugins)), group(group_) {}

  [[nodiscard]] std::string getName() const { return group.name; }
  [[nodiscard]] GroupTypeEnum getType() const { return group.type; }
  [[nodiscard]] std::vector <PluginViewModel> getPlugins() const { return plugins; }
private:
  std::vector<PluginViewModel> plugins;
  const Group group;
};

/*
--------------------------------------------------------------------------------
                                Steps
--------------------------------------------------------------------------------
*/
class StepViewModel {
public:
  StepViewModel(const InstallStep &installStep_, const std::vector<GroupViewModel> &groups)
    : installStep(installStep_), groups(groups) {}

  [[nodiscard]] std::string getName() const { return installStep.name; }
  [[nodiscard]] std::vector<GroupViewModel> getGroups() const { return groups; }

  const InstallStep installStep;
  bool isVisible = true;
  std::vector<GroupViewModel> groups;
};

/*
--------------------------------------------------------------------------------
                                Info
--------------------------------------------------------------------------------
*/
class InfoViewModel {
public:
  explicit InfoViewModel(const std::shared_ptr<FomodInfoFile> &infoFile) {
    if (infoFile) {
      // Copy the necessary members from FomodInfoFile to InfoViewModel
      mName = infoFile->getName();
      mVersion = infoFile->getVersion();
      mAuthor = infoFile->getAuthor();
      mWebsite = infoFile->getWebsite();
    }
  }

  // Accessor methods
  std::string getName() const { return mName; }
  std::string getVersion() const { return mVersion; }
  std::string getAuthor() const { return mAuthor; }
  std::string getWebsite() const { return mWebsite; }

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
class FomodViewModel {
public:

  FomodViewModel(
    MOBase::IOrganizer* organizer,
    const std::shared_ptr<ModuleConfiguration> &fomodFile,
    const std::shared_ptr<FomodInfoFile> &infoFile);

  static std::shared_ptr<FomodViewModel> create(
    MOBase::IOrganizer* organizer,
    const std::shared_ptr<ModuleConfiguration> &fomodFile,
    const std::shared_ptr<FomodInfoFile> &infoFile);


  ~FomodViewModel();

  [[nodiscard]] PluginViewModel &getFirstPluginForActiveStep() const;


  // Steps
  [[nodiscard]] const std::vector<StepViewModel>& getSteps() const { return mSteps; }
  [[nodiscard]] int getCurrentStepIndex() const { return mCurrentStepIndex; }
  void setCurrentStepIndex(const int index) { mCurrentStepIndex = index; }
  [[nodiscard]] bool isStepVisible(int stepIndex) const;

  // Flags
  void setFlag(const std::string &flag, const std::string &value);
  std::string getFlag(const std::string &flag);

  // Plugins
  [[nodiscard]] PluginViewModel* getActivePlugin() const { return mActivePlugin; }

  // Info
  [[nodiscard]] InfoViewModel getInfoViewModel() const { return mInfoViewModel; }

  // Interactions
  void onBackPressed();
  void onNextInstallPressed();
  void collectFlags();

  void togglePlugin(GroupViewModel &group, PluginViewModel &plugin, bool enabled);

  void constructInitialStates();

private:

  // Constructor members
  MOBase::IOrganizer* mOrganizer = nullptr;
  std::shared_ptr<ModuleConfiguration> mFomodFile;
  std::shared_ptr<FomodInfoFile> mInfoFile;
  FlagMap mFlags;
  ConditionTester mConditionTester;

  // Internal only
  InfoViewModel mInfoViewModel;
  std::vector<StepViewModel> mSteps;
  PluginViewModel* mActivePlugin = nullptr;
  StepViewModel* mActiveStep = nullptr;

  // Indices
  int mCurrentStepIndex = 0;

  void createStepViewModels(const std::shared_ptr<ModuleConfiguration> &fomodFile);
};



#endif //DIALOGSTATEMANAGER_H
