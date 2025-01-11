#ifndef FILEINSTALLER_H
#define FILEINSTALLER_H
#include <ifiletree.h>

#include "ConditionTester.h"
#include "FlagMap.h"
#include "xml/ModuleConfiguration.h"


/* This is what legacy fomodInstaller does:
        modName.update(dialog.getName(), GUESS_USER);
        return dialog.updateTree(tree);

   This is a link to the legacy updateTree method:
    https://github.com/ModOrganizer2/modorganizer-installer_fomod/blob/fc263f2d923c704b4853c11ed4f8b8cf3920f30d/src/fomodinstallerdialog.cpp#L546

*/

// Things this should do:
// - Copy all requiredInstall files
// - Copy all conditionalInstall files
// - Copy all selected files from steps that were visible to the user at the point of install

class StepViewModel;

class FileInstaller {
public:
  FileInstaller(
    MOBase::IOrganizer* organizer,
    const QString &fomodPath,
    const std::shared_ptr<MOBase::IFileTree> &fileTree,
    std::unique_ptr<ModuleConfiguration> fomodFile,
    const std::vector<std::shared_ptr<StepViewModel>> &steps);

  std::shared_ptr<MOBase::IFileTree> install() const;

  std::string getQualifiedFilePath(const std::string &treePath) const;

private:
  MOBase::IOrganizer* mOrganizer;
  QString mFomodPath;
  std::shared_ptr<MOBase::IFileTree> mFileTree;
  std::unique_ptr<ModuleConfiguration> mFomodFile;
  FlagMap mFlagMap;
  ConditionTester mConditionTester;
  std::vector<std::shared_ptr<StepViewModel>> mSteps; // TODO: Maybe this is nasty. Idk.

  std::vector<File> collectFilesToInstall() const;
};



#endif //FILEINSTALLER_H
