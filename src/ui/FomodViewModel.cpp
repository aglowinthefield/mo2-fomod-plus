#include "FomodViewModel.h"
#include "xml/ModuleConfiguration.h"

/**
 * 
 * @param organizer
 * @param fomodFile 
 * @param infoFile 
 */
FomodViewModel::FomodViewModel(MOBase::IOrganizer *organizer, const std::shared_ptr<ModuleConfiguration> &fomodFile,
                               const std::shared_ptr<FomodInfoFile> &infoFile)
: mOrganizer(organizer), mFomodFile(fomodFile), mInfoFile(infoFile), mConditionTester(organizer), mInfoViewModel(infoFile) {}

/**
 *
 * @param organizer
 * @param fomodFile
 * @param infoFile
 * @return
 */
std::shared_ptr<FomodViewModel> FomodViewModel::create(MOBase::IOrganizer *organizer,
  const std::shared_ptr<ModuleConfiguration> &fomodFile, const std::shared_ptr<FomodInfoFile> &infoFile) {
  auto viewModel = std::make_shared<FomodViewModel>(organizer, fomodFile, infoFile);
  viewModel->createStepViewModels(fomodFile);
  viewModel->mActiveStep = &viewModel->mSteps.at(0);
  viewModel->mActivePlugin = &viewModel->getFirstPluginForActiveStep();
  return viewModel;
}


FomodViewModel::~FomodViewModel() {
  std::cout << "FomodViewModel destructor called" << std::endl;
}


PluginViewModel &FomodViewModel::getFirstPluginForActiveStep() const {
  if (mSteps.empty()) {
    throw std::runtime_error("No steps found in FomodViewModel");
  }
  return mSteps[mCurrentStepIndex].groups.at(0).getPlugins().at(0);
}

void FomodViewModel::collectFlags() {
  if (mSteps.empty()) {
    return;
  }
  for (auto step : mSteps) {
    for (auto flagDependency : step.installStep.visible.dependencies.flagDependencies) {
      mFlags.setFlag(flagDependency.flag, "");
    }
  }
}

// onpluginselected should also take a group option to set the values for the other plugins, possibly
// TODO: Handle groups later
void FomodViewModel::togglePlugin(GroupViewModel&, PluginViewModel& plugin, bool enabled) {
  plugin.setEnabled(enabled);
  for (auto flag : plugin.getPlugin().conditionFlags.flags) {
    if (enabled) {
      mFlags.setFlag(flag.name, flag.value);
    } else {
      mFlags.setFlag(flag.name, "");
    }
  }
}

void FomodViewModel::constructInitialStates() {
  // For each group, "select" the correct plugin based on the spec.
  for (auto step : mSteps) {
    for (auto group : step.getGroups()) {
      switch (group.getType()) {
        case SelectExactlyOne:
          // Mark the first option that doesn't fail its condition as active
          break;
        case SelectAtLeastOne:
          break;
        case SelectAll:
          // set every plugin in this group to be checked and disabled
            for (auto plugin : group.getPlugins()) {
              togglePlugin(group, plugin, true);
            }
          break;
        // SelectAny, SelectAtMostOne, and don't need anything to be done
        default: ;
      }
    }
  }
}

void FomodViewModel::createStepViewModels(const std::shared_ptr<ModuleConfiguration> &fomodFile) {
  std::vector<StepViewModel> stepViewModels;

  for (const InstallStep& installStep : fomodFile->installSteps.installSteps) {
    std::vector<GroupViewModel> groupViewModels;

    for (const auto& group : installStep.optionalFileGroups.groups) {
      std::vector<PluginViewModel> pluginViewModels;

      for (const auto& plugin : group.plugins.plugins) {
        const auto pluginViewModel = PluginViewModel(plugin, false, true);
        pluginViewModels.emplace_back(pluginViewModel); // Assuming default values for selected and enabled
      }
      const auto groupViewModel = GroupViewModel(group, std::move(pluginViewModels));
      groupViewModels.emplace_back(groupViewModel);
    }
    const auto stepViewModel = StepViewModel(installStep, std::move(groupViewModels));
    stepViewModels.emplace_back(stepViewModel);
  }
  // TODO Sort the view models here, maybe
  collectFlags();

  mSteps = stepViewModels;
}

bool FomodViewModel::isStepVisible(const int stepIndex) const {
  const auto step = mSteps[stepIndex].installStep;
  return mConditionTester.isStepVisible(mFlags, step);
}

void FomodViewModel::onBackPressed() {
  if (mCurrentStepIndex <= 0) {
    return;
  }

  // TODO: go back to previously visible step
  // we need to set up the initial state of the viewmodel first.
  mCurrentStepIndex--;
}

void FomodViewModel::onNextInstallPressed() {
}

void FomodViewModel::setFlag(const std::string &flag, const std::string &value) {
  mFlags.setFlag(flag, value);
}

std::string FomodViewModel::getFlag(const std::string &flag) {
  return mFlags.getFlag(flag);
}

