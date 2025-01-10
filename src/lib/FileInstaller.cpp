#include "FileInstaller.h"

FileInstaller::FileInstaller(
  const std::shared_ptr<MOBase::IFileTree> &fileTree,
  std::unique_ptr<ModuleConfiguration> fomodFile) : mFileTree(fileTree), mFomodFile(std::move(fomodFile)) {}

std::shared_ptr<MOBase::IFileTree> FileInstaller::install() {
}

std::vector<File> FileInstaller::collectFilesToInstall() {

  std::vector<File> filesToInstall;

  // Required files from FOMOD
  FileList requiredInstallFiles = mFomodFile->requiredInstallFiles;
  filesToInstall.insert(filesToInstall.end(), requiredInstallFiles.files.begin(), requiredInstallFiles.files.end());

  // Conditional install files

  return filesToInstall;
};
