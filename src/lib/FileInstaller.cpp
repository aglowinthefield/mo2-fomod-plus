#include "FileInstaller.h"

#include "ui/FomodViewModel.h"

FileInstaller::FileInstaller(
  MOBase::IOrganizer *organizer,
  const std::shared_ptr<MOBase::IFileTree> &fileTree,
  std::unique_ptr<ModuleConfiguration> fomodFile,
  const std::vector<std::shared_ptr<StepViewModel> > &steps) : mOrganizer(organizer), mFileTree(fileTree),
                                                               mFomodFile(std::move(fomodFile)),
                                                               mConditionTester(organizer), mSteps(steps) {
}

std::shared_ptr<MOBase::IFileTree> FileInstaller::install() {
  return nullptr;
}

// Generic vector appender
void addFiles(std::vector<File> main, std::vector<File> toAdd) {
  main.insert(main.end(), toAdd.begin(), toAdd.end());
}

// TODO: Unclear if we're copying. oh well.
std::vector<File> FileInstaller::collectFilesToInstall() const {
  std::vector<File> filesToInstall;

  // Required files from FOMOD
  const FileList requiredInstallFiles = mFomodFile->requiredInstallFiles;
  addFiles(filesToInstall, requiredInstallFiles.files);

  // ConditionalInstall files
  const auto conditionalInstalls = mFomodFile->conditionalFileInstalls;
  for (auto pattern: conditionalInstalls.patterns) {
    if (mConditionTester.testCompositeDependency(mFlagMap, pattern.dependencies)) {
      addFiles(filesToInstall, pattern.files.files);
    }
  }

  // Selected files from visible steps
  for (const auto stepViewModel : mSteps) {
    if (!mConditionTester.isStepVisible(mFlagMap, stepViewModel->installStep.get())) {
      continue;
    }
    for (const auto groupViewModel : stepViewModel->getGroups()) {
      for (const auto pluginViewModel : groupViewModel->getPlugins()) {
        if (pluginViewModel->isSelected()) {
          addFiles(filesToInstall, pluginViewModel->getPlugin()->files.files);
        }
      }
    }
  }

  return filesToInstall;
}
