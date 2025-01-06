#ifndef FOMODINSTALLERWINDOW_H
#define FOMODINSTALLERWINDOW_H

#include <qboxlayout.h>
#include <qcombobox.h>

#include "InstallerFomodPlus.h"
#include "xml/ModuleConfiguration.h"
#include "xml/FomodInfoFile.h"

#include <QDialog>
#include <QWidget>
#include <QStackedWidget>

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


  // So InstallerFomodPlus can check if the user wants to manually install
  [[nodiscard]] bool getIsManualInstall() const {
    return mIsManualInstall;
  }

private slots:
  void onNextClicked();
  void onBackClicked();
  void onInstallClicked() { this->accept(); }
  void onCancelClicked() { this->reject(); }
  void onManualClicked() { mIsManualInstall = true; this->reject(); }

private:
  InstallerFomodPlus *mInstaller;
  GuessedValue<QString> mModName;
  std::shared_ptr<IFileTree> mTree;
  std::unique_ptr<ModuleConfiguration> mFomodFile;
  std::unique_ptr<FomodInfoFile> mInfoFile;

  // Meta
  bool mIsManualInstall{};

  // Buttons
  QWidget* mNextInstallButton{};
  QWidget* mBackButton{};
  QWidget* mCancelButton{};
  QWidget* mManualButton{};
  void updateButtons();

  // Widgets
  QStackedWidget* mInstallStepStack{};
  QWidget* mLeftPane{};
  int mCurrentStepIndex{};

  // Fn
  void setupUi();
  void updateInstallStepStack();

  /**
   * Render the outer container which will have:
   *   - the left pane (containing the selected plugin / page info and image)
   *   - the right pane (containing the step, groups, and plugins)
   *   - the bottom row for various data + buttons
   */
  QBoxLayout *createContainerLayout();

  QWidget *createCenterRow();

  QWidget *createTopRow();

  QComboBox *createModNameComboBox();

  [[nodiscard]] QWidget* createBottomRow();

  /**
   * A step will have a name, a list of groups, and a list of plugins.
   * It will also be in charge of rendering a left pane with some arguments TODO
   *
   */
  QWidget* createStepWidget(const InstallStep& installStep);

  /**
   * A group will have a name, a type, and a list of plugins.
   *
   */
  QWidget* renderGroup(Group &group);

  /**
   * A plugin will have a name, a description, an image, and a type descriptor.
   * It will have conditions TODO
   *
   */
  QWidget* renderPlugin(Plugin &plugin); // Plugins will emit a signal to

};



#endif //FOMODINSTALLERWINDOW_H
