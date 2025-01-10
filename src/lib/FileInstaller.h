#ifndef FILEINSTALLER_H
#define FILEINSTALLER_H
#include <ifiletree.h>

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

class FileInstaller {
public:
  FileInstaller(const std::shared_ptr<MOBase::IFileTree> &fileTree, std::unique_ptr<ModuleConfiguration> fomodFile);
  std::shared_ptr<MOBase::IFileTree> install();

private:
  std::shared_ptr<MOBase::IFileTree> mFileTree;
  std::unique_ptr<ModuleConfiguration> mFomodFile;
};



#endif //FILEINSTALLER_H
