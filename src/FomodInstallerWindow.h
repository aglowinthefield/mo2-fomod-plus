#ifndef FOMODINSTALLERWINDOW_H
#define FOMODINSTALLERWINDOW_H

#include <qboxlayout.h>
#include <qbuttongroup.h>
#include <qcheckbox.h>
#include <qcombobox.h>

#include "InstallerFomodPlus.h"
#include "xml/ModuleConfiguration.h"

#include <QDialog>
#include <qradiobutton.h>
#include <QStackedWidget>
#include <QTextEdit>
#include <ui/ScaleLabel.h>

#include "FomodInstallerWindow.h"
#include "lib/FileInstaller.h"
#include "ui/FomodViewModel.h"

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
class FomodInstallerWindow final : public QDialog {
  Q_OBJECT
public:
  FomodInstallerWindow(InstallerFomodPlus *installer,
                       GuessedValue<QString> &modName,
                       const std::shared_ptr<IFileTree> &tree,
                       QString fomodPath,
                       const std::shared_ptr<FomodViewModel> &viewModel,
                       QWidget *parent = nullptr);


  // So InstallerFomodPlus can check if the user wants to manually install
  [[nodiscard]] bool isManualInstall() const {
    return mIsManualInstall;
  }
  [[nodiscard]] std::shared_ptr<FileInstaller> getFileInstaller() const { return mViewModel->getFileInstaller(); }

private slots:
  void onNextClicked();
  void onPluginToggled(bool selected, const std::shared_ptr<GroupViewModel> &group, const std::shared_ptr<PluginViewModel> &plugin) const;
  void updateModName(const QString& name) const {
    mModName.update(name, GUESS_USER);
  }


  void onBackClicked() const;
  void onCancelClicked() { this->reject(); }
  void onManualClicked() { mIsManualInstall = true; this->reject(); }
  void onInstallClicked();


private:
  InstallerFomodPlus *mInstaller;
  QString mFomodPath;
  GuessedValue<QString>& mModName;
  std::shared_ptr<IFileTree> mTree;
  std::shared_ptr<FomodViewModel> mViewModel;

  // Meta
  bool mIsManualInstall{};

  // Buttons
  QPushButton* mNextInstallButton{};
  QPushButton* mBackButton{};
  QPushButton* mCancelButton{};
  QPushButton* mManualButton{};
  void updateButtons() const;

  // Widgets
  QStackedWidget* mInstallStepStack{};
  QWidget* mLeftPane{};
  QTextEdit* mDescriptionBox{};
  QComboBox* mModNameInput{};
  ScaleLabel* mImageLabel{};

  // Fn
  void setupUi();
  void updateInstallStepStack();
  void updateDisplayForActivePlugin() const;

  [[nodiscard]] QBoxLayout* createContainerLayout();
  [[nodiscard]] QWidget*    createCenterRow();
  [[nodiscard]] QWidget*    createTopRow();
  [[nodiscard]] QComboBox*  createModNameComboBox();
  [[nodiscard]] QWidget*    createBottomRow();
  [[nodiscard]] QWidget*    createLeftPane();
  [[nodiscard]] QWidget*    createRightPane();
  [[nodiscard]] QWidget*    createStepWidget(const std::shared_ptr<StepViewModel> &installStep);
  [[nodiscard]] QWidget*    renderGroup(const std::shared_ptr<GroupViewModel> &group);

  QRadioButton *createPluginRadioButton(const std::shared_ptr<PluginViewModel> &plugin, const std::shared_ptr<GroupViewModel> &group, QWidget *parent);

  QCheckBox *createPluginCheckBox(const std::shared_ptr<PluginViewModel> &plugin,
                                  const std::shared_ptr<GroupViewModel> &group, QWidget *parent);

  void renderSelectAtLeastOne(QWidget *parent, QLayout *parentLayout,
                              const std::shared_ptr<GroupViewModel> &group);

  QButtonGroup *renderSelectExactlyOne(QWidget *parent, QLayout *parentLayout, const std::shared_ptr<GroupViewModel> &group);

  void renderSelectAtMostOne(QWidget *parent, QLayout *parentLayout, const std::shared_ptr<GroupViewModel> &group);
  void renderSelectAny(QWidget *parent, QLayout *parentLayout, const std::shared_ptr<GroupViewModel> &group);
};



#endif //FOMODINSTALLERWINDOW_H
