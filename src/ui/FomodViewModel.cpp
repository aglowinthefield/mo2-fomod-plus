#include "FomodViewModel.h"
#include "xml/ModuleConfiguration.h"

/*
--------------------------------------------------------------------------------
                               Lifecycle
--------------------------------------------------------------------------------
*/

/**
 * @brief FomodViewModel constructor
 *
 * @note DO NOT USE DIRECTLY. We should only use FomodViewModel::create() to create a new instance.
 * @see FomodViewModel::create
 *
 * @param organizer The organizer instance passed from the IInstaller
 * @param fomodFile The ModuleConfiguration instance created from the raw ModuleConfiguration.xml file
 * @param infoFile  The FomodInfoFile instance created from the raw info.xml file
 *
 * @return A FomodViewModel instance
 */
FomodViewModel::FomodViewModel(MOBase::IOrganizer *organizer,
                               std::unique_ptr<ModuleConfiguration> fomodFile,
                               std::unique_ptr<FomodInfoFile> infoFile)
  : mOrganizer(organizer), mFomodFile(std::move(fomodFile)), mInfoFile(std::move(infoFile)),
    mConditionTester(organizer),
    mInfoViewModel(std::move(infoFile)) {
}

/**
 *
 * @param organizer The organizer instance passed from the IInstaller
 * @param fomodFile The ModuleConfiguration instance created from the raw ModuleConfiguration.xml file
 * @param infoFile  The FomodInfoFile instance created from the raw info.xml file
 * @return A shared pointer to the FomodViewModel instance
 */
std::shared_ptr<FomodViewModel> FomodViewModel::create(MOBase::IOrganizer *organizer,
                                                       std::unique_ptr<ModuleConfiguration> fomodFile,
                                                       std::unique_ptr<FomodInfoFile> infoFile) {
  auto viewModel = std::make_shared<FomodViewModel>(organizer, std::move(fomodFile), std::move(infoFile));
  viewModel->createStepViewModels();
  viewModel->mActiveStep = viewModel->mSteps.at(0);
  viewModel->mActivePlugin = viewModel->getFirstPluginForActiveStep();
  std::cout << "Active Plugin: " << viewModel->mActivePlugin->getName() << std::endl;
  return viewModel;
}

/*
--------------------------------------------------------------------------------
                               Initialization
--------------------------------------------------------------------------------
*/

void FomodViewModel::constructInitialStates() {
  // For each group, "select" the correct plugin based on the spec.
  for (const auto& step : mSteps) {
    for (const auto& group : step->getGroups()) {
      switch (group->getType()) {
        case SelectExactlyOne:
          // Mark the first option that doesn't fail its condition as active
          for (const auto& plugin : group->getPlugins()) {
            if (mConditionTester.getPluginTypeDescriptorState(plugin->getPlugin(), mFlags) != PluginTypeEnum::NotUsable) {
              togglePlugin(group, plugin, true);
              break;
            }
          }
          break;
        case SelectAtLeastOne:
          if (group->getPlugins().size() == 1) {
            group->getPlugins()[0]->setEnabled(false);
            togglePlugin(group, group->getPlugins()[0], true);
            break;
          }
          break;
        case SelectAll:
          // set every plugin in this group to be checked and disabled
            for (const auto& plugin : group->getPlugins()) {
              togglePlugin(group, plugin, true);
              plugin->setEnabled(false);
            }
          break;
        // SelectAny, SelectAtMostOne, and don't need anything to be done
        default: ;
      }
    }
  }
}

void FomodViewModel::processPluginConditions() {
  for (const auto stepViewModel : mSteps) {
    for (const auto groupViewModel : stepViewModel->getGroups()) {
      for (auto pluginViewModel : groupViewModel->getPlugins()) {
        const auto typeDescriptor = mConditionTester.getPluginTypeDescriptorState(pluginViewModel->getPlugin(), mFlags);
        if (typeDescriptor == PluginTypeEnum::NotUsable) {
          std::cout << "Plugin is not usable: [" << pluginViewModel->getName() << "]" << std::endl;
          pluginViewModel->setEnabled(false);
          togglePlugin(groupViewModel, pluginViewModel, false);
        }
        if (typeDescriptor == PluginTypeEnum::Recommended) {
          pluginViewModel->setEnabled(true);
          togglePlugin(groupViewModel, pluginViewModel, true);
          std::cout << "Plugin is recommended: [" << pluginViewModel->getName() << "]" << std::endl;
        }
        if (typeDescriptor == PluginTypeEnum::Required) {
          pluginViewModel->setEnabled(false);
          togglePlugin(groupViewModel, pluginViewModel, true);
          std::cout << "Plugin is required: [" << pluginViewModel->getName() << "]" << std::endl;
        }
      }
    }
  }

}

void FomodViewModel::createStepViewModels() {
  shared_ptr_list<StepViewModel> stepViewModels;

  for (const auto& installStep : mFomodFile->installSteps.installSteps) {
    std::vector<std::shared_ptr<GroupViewModel>> groupViewModels;

    for (const auto& group : installStep.optionalFileGroups.groups) {
      std::vector<std::shared_ptr<PluginViewModel>> pluginViewModels;

      for (const auto& plugin : group.plugins.plugins) {
        auto pluginViewModel = std::make_shared<PluginViewModel>(std::make_shared<Plugin>(plugin), false, true);
        pluginViewModels.emplace_back(pluginViewModel); // Assuming default values for selected and enabled
      }
      auto groupViewModel = std::make_shared<GroupViewModel>(std::make_shared<Group>(group), std::move(pluginViewModels));
      groupViewModels.emplace_back(groupViewModel);
    }
    auto stepViewModel = std::make_shared<StepViewModel>(std::make_shared<InstallStep>(installStep), std::move(groupViewModels));
    stepViewModels.emplace_back(stepViewModel);
  }
  // TODO Sort the view models here, maybe
  mSteps = std::move(stepViewModels);
  updateVisibleSteps();
  constructInitialStates();
  processPluginConditions();
}

// TODO: Handle groups later
void FomodViewModel::togglePlugin(const std::shared_ptr<GroupViewModel>&, const std::shared_ptr<PluginViewModel> &plugin, const bool selected) {
  plugin->setSelected(selected);
  for (const auto& flag : plugin->getPlugin()->conditionFlags.flags) {
    mFlags.setFlag(flag.name, selected ? flag.value : "");
  }
  mActivePlugin = plugin;
  updateVisibleSteps();
}


/*
--------------------------------------------------------------------------------
                               Step Visibility
--------------------------------------------------------------------------------
*/
bool FomodViewModel::isStepVisible(const int stepIndex) const {
  const auto step = mSteps[stepIndex]->installStep;
  return mConditionTester.isStepVisible(mFlags, step.get());
}

void FomodViewModel::updateVisibleSteps() {
  mVisibleStepIndices.clear();
  for (int i = 0; i < mSteps.size(); ++i) {
    if (isStepVisible(i)) {
      mVisibleStepIndices.push_back(i);
    }
  }
}

void FomodViewModel::preinstall(const std::shared_ptr<MOBase::IFileTree> &tree, const QString &fomodPath) {
  mFileInstaller = std::make_shared<FileInstaller>(mOrganizer, fomodPath, tree, std::move(mFomodFile), mFlags, mSteps);
}

bool FomodViewModel::isLastVisibleStep() const {
  return !mVisibleStepIndices.empty() && mCurrentStepIndex == mVisibleStepIndices.back();
}

/*
--------------------------------------------------------------------------------
                               Navigation
--------------------------------------------------------------------------------
*/
void FomodViewModel::stepBack() {
  const auto it = std::ranges::find(mVisibleStepIndices, mCurrentStepIndex);
  if (it != mVisibleStepIndices.end() && it != mVisibleStepIndices.begin()) {
    mCurrentStepIndex = *std::prev(it);
    mActiveStep = mSteps[mCurrentStepIndex];
    mActivePlugin = getFirstPluginForActiveStep();
  }
}

void FomodViewModel::stepForward() {
  const auto it = std::ranges::find(mVisibleStepIndices, mCurrentStepIndex);
  if (it != mVisibleStepIndices.end() && std::next(it) != mVisibleStepIndices.end()) {
    mCurrentStepIndex = *std::next(it);
    mActiveStep = mSteps[mCurrentStepIndex];
    mActivePlugin = getFirstPluginForActiveStep();
  }
}

/*
--------------------------------------------------------------------------------
                               Flags
--------------------------------------------------------------------------------
*/
void FomodViewModel::setFlag(const std::string &flag, const std::string &value) {
  mFlags.setFlag(flag, value);
}

std::string FomodViewModel::getFlag(const std::string &flag) {
  return mFlags.getFlag(flag);
}

/*
--------------------------------------------------------------------------------
                               Display
--------------------------------------------------------------------------------
*/
std::string FomodViewModel::getDisplayImage() const {
  // if the active plugin has an image, return it
  if (!mActivePlugin->getImagePath().empty()) {
    return mActivePlugin->getImagePath();
  }

  // otherwise show the default fomod image if we're on step 1.
  if (mCurrentStepIndex == 0) {
    return mFomodFile->moduleImage.path;
  }

  // otherwise nothin'
  return "";
}
