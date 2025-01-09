#ifndef FOMODINSTALLERWINDOW_H
#define FOMODINSTALLERWINDOW_H

#include <qboxlayout.h>
#include <qbuttongroup.h>
#include <qcombobox.h>

#include "InstallerFomodPlus.h"
#include "xml/ModuleConfiguration.h"
#include "xml/FomodInfoFile.h"

#include <QDialog>
#include <QWidget>
#include <QStackedWidget>
#include <QTextEdit>
#include <ui/ScaleLabel.h>

#include "FomodInstallerWindow.h"
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
                       const GuessedValue<QString> &modName,
                       const std::shared_ptr<IFileTree> &tree,
                       const QString &fomodPath,
                       const std::shared_ptr<FomodViewModel> &viewModel,
                       QWidget *parent = nullptr);


  // So InstallerFomodPlus can check if the user wants to manually install
  [[nodiscard]] bool isManualInstall() const {
    return mIsManualInstall;
  }

private slots:
  void onNextClicked();

  void updateNextVisibleStepIndex();

  void onBackClicked() const;
  void onInstallClicked() { this->accept(); }
  void onCancelClicked() { this->reject(); }
  void onManualClicked() { mIsManualInstall = true; this->reject(); }

private:
  InstallerFomodPlus *mInstaller;
  QString mFomodPath;
  GuessedValue<QString> mModName;
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
  [[nodiscard]] QWidget*    createStepWidget(const StepViewModel &installStep);
  [[nodiscard]] QWidget*    renderGroup(const GroupViewModel *group);

  static QButtonGroup *renderSelectExactlyOne(QWidget *parent, QLayout *parentLayout, const GroupViewModel &group);

  static void renderSelectAtMostOne(QWidget *parent, QLayout *parentLayout, const GroupViewModel &group);
  static void renderSelectAny(QWidget *parent, QLayout *parentLayout, const GroupViewModel &group);
};



#endif //FOMODINSTALLERWINDOW_H
