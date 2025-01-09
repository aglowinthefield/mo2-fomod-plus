#include "FomodViewModel.h"
#include "xml/ModuleConfiguration.h"

/**
 * 
 * @param organizer
 * @param fomodFile 
 * @param infoFile 
 */
FomodViewModel::FomodViewModel(MOBase::IOrganizer *organizer,
                               std::unique_ptr<ModuleConfiguration> fomodFile,
                               std::unique_ptr<FomodInfoFile> infoFile)
: mOrganizer(organizer), mFomodFile(std::move(fomodFile)), mInfoFile(std::move(infoFile)), mConditionTester(organizer),
  mInfoViewModel(std::move(infoFile)) {
}

/**
 *
 * @param organizer
 * @param fomodFile
 * @param infoFile
 * @return
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


FomodViewModel::~FomodViewModel() {
  std::cout << "FomodViewModel destructor called" << std::endl;
}


std::shared_ptr<PluginViewModel> FomodViewModel::getFirstPluginForActiveStep() const {
  if (mSteps.empty()) {
    throw std::runtime_error("No steps found in FomodViewModel");
  }
  return mActiveStep->getGroups().at(0)->getPlugins().at(0);
}

void FomodViewModel::collectFlags() {
  if (mSteps.empty()) {
    return;
  }
  for (const auto step : mSteps) {
    for (auto flagDependency : step->installStep->visible.dependencies.flagDependencies) {
      mFlags.setFlag(flagDependency.flag, "");
    }
  }
}

// onpluginselected should also take a group option to set the values for the other plugins, possibly
// TODO: Handle groups later
void FomodViewModel::togglePlugin(std::shared_ptr<GroupViewModel>, const std::shared_ptr<PluginViewModel> &plugin, const bool enabled) {
  plugin->setEnabled(enabled);
  for (auto flag : plugin->getPlugin()->conditionFlags.flags) {
    if (enabled) {
      mFlags.setFlag(flag.name, flag.value);
    } else {
      mFlags.setFlag(flag.name, "");
    }
  }
}

void FomodViewModel::constructInitialStates() {
  // For each group, "select" the correct plugin based on the spec.
  for (const auto step : mSteps) {
    for (auto group : step->getGroups()) {
      switch (group->getType()) {
        case SelectExactlyOne:
          // Mark the first option that doesn't fail its condition as active
          break;
        case SelectAtLeastOne:
          break;
        case SelectAll:
          // set every plugin in this group to be checked and disabled
            for (auto plugin : group->getPlugins()) {
              togglePlugin(group, plugin, true);
            }
          break;
        // SelectAny, SelectAtMostOne, and don't need anything to be done
        default: ;
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
  collectFlags();
  mSteps = std::move(stepViewModels);
}

bool FomodViewModel::isStepVisible(const int stepIndex) const {
  const auto step = mSteps[stepIndex]->installStep;
  return mConditionTester.isStepVisible(mFlags, step.get());
}

void FomodViewModel::stepBack() {
  if (mCurrentStepIndex <= 0) {
    return;
  }

  // TODO: go back to previously visible step
  // we need to set up the initial state of the viewmodel first.
  while (!isStepVisible(mCurrentStepIndex - 1)) {
    mCurrentStepIndex--;
  }
  mActiveStep = mSteps[mCurrentStepIndex];
  mNextOp = (mCurrentStepIndex == mSteps.size() - 1) ? NEXT_OP::INSTALL : NEXT_OP::NEXT;
}

void FomodViewModel::stepForward() {
  if (mCurrentStepIndex >= mSteps.size() - 1) {
    return;
  }

  // While step isn't visible, increment
  while (!isStepVisible(mCurrentStepIndex + 1) && mCurrentStepIndex < mSteps.size() - 1) {
    mCurrentStepIndex++;
  }
  mActiveStep = mSteps[mCurrentStepIndex];
  mNextOp = (mCurrentStepIndex == mSteps.size() - 1) ? NEXT_OP::INSTALL : NEXT_OP::NEXT;
}

void FomodViewModel::setFlag(const std::string &flag, const std::string &value) {
  mFlags.setFlag(flag, value);
}

std::string FomodViewModel::getFlag(const std::string &flag) {
  return mFlags.getFlag(flag);
}

