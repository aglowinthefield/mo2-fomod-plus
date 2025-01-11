#ifndef DIALOGSTATEMANAGER_H
#define DIALOGSTATEMANAGER_H
#include <imoinfo.h>
#include <string>

#include "../lib/FlagMap.h"
#include "../lib/ConditionTester.h"
#include "lib/FileInstaller.h"
#include "xml/FomodInfoFile.h"

template<typename T>
using shared_ptr_list = std::vector<std::shared_ptr<T>>;

/*
--------------------------------------------------------------------------------
                                Plugins
--------------------------------------------------------------------------------
*/
class PluginViewModel {
public:
  PluginViewModel(std::shared_ptr<Plugin> plugin_, const bool selected, bool)
    : plugin(std::move(plugin_)), selected(selected), enabled(true) {
  }

  void setSelected(const bool selected) { this->selected = selected; }
  void setEnabled(const bool enabled)   { this->enabled = enabled; }
  [[nodiscard]] std::string getName() const               { return plugin->name; }
  [[nodiscard]] std::string getDescription() const        { return plugin->description; }
  [[nodiscard]] std::string getImagePath() const          { return plugin->image.path; }
  [[nodiscard]] bool isSelected() const                   { return selected; }
  [[nodiscard]] bool isEnabled() const                    { return enabled; }
  [[nodiscard]] std::shared_ptr<Plugin> getPlugin() const { return plugin; }

private:
  const std::shared_ptr<Plugin> plugin;
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
  GroupViewModel(std::shared_ptr<Group> group_, shared_ptr_list<PluginViewModel> plugins)
    : plugins(std::move(plugins)), group(std::move(group_)) {}

  [[nodiscard]] std::string getName() const { return group->name; }
  [[nodiscard]] GroupTypeEnum getType() const { return group->type; }
  [[nodiscard]] shared_ptr_list<PluginViewModel> getPlugins() const { return plugins; }
private:
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
  StepViewModel(std::shared_ptr<InstallStep> installStep_, const shared_ptr_list<GroupViewModel> &groups)
    : installStep(std::move(installStep_)), groups(groups) {}

  [[nodiscard]] std::string getName() const { return installStep->name; }
  [[nodiscard]] shared_ptr_list<GroupViewModel> getGroups() const { return groups; }

  const std::shared_ptr<InstallStep> installStep;
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
  [[nodiscard]] std::string getName() const    { return mName; }
  [[nodiscard]] std::string getVersion() const { return mVersion; }
  [[nodiscard]] std::string getAuthor() const  { return mAuthor; }
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
    MOBase::IOrganizer *organizer,
    std::unique_ptr<ModuleConfiguration> fomodFile,
    std::unique_ptr<FomodInfoFile> infoFile);

  [[nodiscard]] std::shared_ptr<PluginViewModel> getFirstPluginForActiveStep() const {
    return mActiveStep->getGroups().at(0)->getPlugins().at(0);
  }

  // Steps
  [[nodiscard]] shared_ptr_list<StepViewModel> getSteps() const { return mSteps; }
  [[nodiscard]] int getCurrentStepIndex() const { return mCurrentStepIndex; }
  [[deprecated]] void setCurrentStepIndex(const int index) { mCurrentStepIndex = index; }
  [[nodiscard]] bool isStepVisible(int stepIndex) const;

  void updateVisibleSteps();
  void preinstall(const std::shared_ptr<MOBase::IFileTree> &tree, const QString& fomodPath);
  std::shared_ptr<FileInstaller> getFileInstaller() { return mFileInstaller; }

  // Flags
  void setFlag(const std::string &flag, const std::string &value);
  std::string getFlag(const std::string &flag);

  std::string getDisplayImage() const;

  // Plugins
  [[nodiscard]] std::shared_ptr<PluginViewModel> getActivePlugin() const { return mActivePlugin; }

  // Info
  [[nodiscard]] InfoViewModel getInfoViewModel() const { return mInfoViewModel; }

  // Interactions
  void stepBack();
  void stepForward();
  bool isLastVisibleStep() const;
  void togglePlugin(const std::shared_ptr<GroupViewModel>&, const std::shared_ptr<PluginViewModel> &plugin, bool selected);
  void setActivePlugin(const std::shared_ptr<PluginViewModel> &plugin) { mActivePlugin = plugin; }

private:

  // Constructor members
  MOBase::IOrganizer* mOrganizer = nullptr;
  std::unique_ptr<ModuleConfiguration> mFomodFile;
  std::unique_ptr<FomodInfoFile> mInfoFile;
  FlagMap mFlags;
  ConditionTester mConditionTester;

  // Internal only
  // TODO: This is a LOT of shared_ptr nonsense. It works for now but I need to understand it better to fix it.
  InfoViewModel mInfoViewModel;
  std::vector<std::shared_ptr<StepViewModel>> mSteps;
  std::shared_ptr<PluginViewModel> mActivePlugin = nullptr; // TODO: This will update on hover and click
  std::shared_ptr<StepViewModel> mActiveStep = nullptr;
  std::vector<int> mVisibleStepIndices;
  std::shared_ptr<FileInstaller> mFileInstaller = nullptr;

  void createStepViewModels();
  void collectFlags();
  void constructInitialStates();
  void processPluginConditions();

  // Indices
  int mCurrentStepIndex{0};

};



#endif //DIALOGSTATEMANAGER_H
