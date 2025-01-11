#include "FileInstaller.h"

#include "ui/FomodViewModel.h"

using namespace MOBase;

FileInstaller::FileInstaller(
  IOrganizer *organizer,
  const QString &fomodPath,
  const std::shared_ptr<IFileTree> &fileTree,
  std::unique_ptr<ModuleConfiguration> fomodFile,
  const std::vector<std::shared_ptr<StepViewModel> > &steps) : mOrganizer(organizer), mFomodPath(fomodPath),
                                                               mFileTree(fileTree),
                                                               mFomodFile(std::move(fomodFile)),
                                                               mConditionTester(organizer), mSteps(steps) {
}

std::shared_ptr<IFileTree> FileInstaller::install() const {
  const auto filesToInstall = collectFilesToInstall();
  std::cout << "Installing " << filesToInstall.size() << " files" << std::endl;

  // update the file tree with the new files
  std::shared_ptr<IFileTree> installTree = mFileTree->createOrphanTree();

  // We need to prepend the fomod path to whatever source we reference. Guess we're passing that path around.

  // VERY BASIC INSTALLATION
  // TODO: Needs lots of work. Just want to see default behaviors first
  for (const auto& file : filesToInstall) {
    if (file.isFolder) {
      const auto path = getQualifiedFilePath(file.source);
      const auto sourceDir = mFileTree->findDirectory(QString::fromStdString(path));
      if (sourceDir == nullptr) {
        std::cerr << "Could not find source folder: " << file.source << std::endl;
        continue; // TODO: Return errors somehow. Maybe in a pair of filetree and errors
      }
      const auto targetNode = installTree->addDirectory(QString::fromStdString(file.destination));

      sourceDir->walk(
        [&targetNode](QString const &currentPath, const std::shared_ptr<const FileTreeEntry> &entry) {
        std::cout << "Walking: " << currentPath.toStdString() << std::endl;
        std::cout << "Looking at: " << entry->name().toStdString() << std::endl;
        if (!entry->isDir()) {
          targetNode->copy(entry, ""); // need to do merge policy after overwrites/priorities are settled
        }
        return IFileTree::WalkReturn::CONTINUE;
      });

    }
  }
  return installTree;
}

std::string FileInstaller::getQualifiedFilePath(const std::string &treePath) const {
  return mFomodPath.toStdString() + "/" + treePath;
}

// Generic vector appender
void addFiles(std::vector<File>& main, std::vector<File> toAdd) {
  for (const auto& add : toAdd) {
    std::cout << "Adding file with source: " << add.source << std::endl;
  }
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
  for (const auto& pattern: conditionalInstalls.patterns) {
    if (mConditionTester.testCompositeDependency(mFlagMap, pattern.dependencies)) {
      addFiles(filesToInstall, pattern.files.files);
    }
  }

  // Selected files from visible steps
  for (const auto& stepViewModel : mSteps) {
    if (!mConditionTester.isStepVisible(mFlagMap, stepViewModel->installStep.get())) {
      continue;
    }
    for (const auto& groupViewModel : stepViewModel->getGroups()) {
      for (const auto& pluginViewModel : groupViewModel->getPlugins()) {
        if (pluginViewModel->isSelected()) {
          addFiles(filesToInstall, pluginViewModel->getPlugin()->files.files);
        }
      }
    }
  }

  return filesToInstall;
}
