#ifndef FOMODINSTALLERWINDOW_H
#define FOMODINSTALLERWINDOW_H

#include "InstallerFomodPlus.h"
#include "xml/ModuleConfiguration.h"
#include "xml/FomodInfoFile.h"

#include <QDialog>

#include "FomodInstallerWindow.h"

using namespace MOBase;

namespace Ui {
  class FomodInstallerWindow;
}

class InstallerFomodPlus;
/**
 * @class FomodInstallerWindow
 * @brief This class represents a window for the FOMOD installer.
 *
 * The FomodInstallerWindow class is designed to handle and manage the FOMOD installation
 * process. It integrates functionalities specific to FOMOD package installations.
 * By inheriting from QObject, it supports signal-slot mechanisms, enabling interaction
 * and communication with other components in the application.
 *
 * This class is primarily intended to provide a user interface and functionality
 * to process and run the FOMOD installer in a structured manner.
 */
class FomodInstallerWindow : public QDialog {
  Q_OBJECT
public:
  FomodInstallerWindow(InstallerFomodPlus *installer,
                       const GuessedValue<QString> &modName,
                       const std::shared_ptr<IFileTree> &tree,
                       std::unique_ptr<ModuleConfiguration> fomodFile,
                       std::unique_ptr<FomodInfoFile> infoFile,
                       QWidget *parent = nullptr);

private:
  InstallerFomodPlus *mInstaller;
  GuessedValue<QString> mModName;
  std::shared_ptr<IFileTree> mTree;
  std::unique_ptr<ModuleConfiguration> mFomodFile;
  std::unique_ptr<FomodInfoFile> mInfoFile;
};



#endif //FOMODINSTALLERWINDOW_H
