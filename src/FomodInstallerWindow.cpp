#include "FomodInstallerWindow.h"
#include "ui/UIHelper.h"

#include <QVBoxLayout>

/**
 * 
 * @param installer 
 * @param modName 
 * @param tree 
 * @param fomodFile 
 * @param infoFile 
 * @param parent 
 */
FomodInstallerWindow::FomodInstallerWindow(InstallerFomodPlus *installer, const GuessedValue<QString> &modName,
  const std::shared_ptr<IFileTree> &tree, std::unique_ptr<ModuleConfiguration> fomodFile,
  std::unique_ptr<FomodInfoFile> infoFile, QWidget *parent): QDialog(parent),
                                                             mInstaller(installer),
                                                             mModName(modName),
                                                             mTree(tree),
                                                             mFomodFile(std::move(fomodFile)),
                                                             mInfoFile(std::move(infoFile)) {

  setupUi();

  const auto containerLayout = createContainerLayout();
  setLayout(containerLayout);

}

void FomodInstallerWindow::setupUi() {

  setMinimumSize(UiConstants::WINDOW_MIN_WIDTH, UiConstants::WINDOW_MIN_HEIGHT);
  setWindowModality(Qt::NonModal); // To allow scrolling modlist without closing the window

}

/*
+-------------------------------------------------------------------+
| +----------------------------------------------------------------+|
| |                                                                ||
| |                    Metadata and Name Input                     ||
| |                                                                ||
| +----------------------------------------------------------------+|
| +------------------------------++--------------------------------+|
| |                              ||                                ||
| |                              ||                                ||
| |         Description          ||                                ||
| |                              ||                                ||
| |                              ||        Step/Group/Plugins      ||
| |                              ||                                ||
| +------------------------------+|                                ||
| +------------------------------+|                                ||
| |                              ||                                ||
| |                              ||                                ||
| |            Image             ||                                ||
| |                              ||                                ||
| |                              ||                                ||
| |                              ||                                ||
| +----------------------------------------------------------------+|
| |                                                                ||
| |                          Bottom Bar                            ||
| +----------------------------------------------------------------+|
+-------------------------------------------------------------------+
*/
QBoxLayout *FomodInstallerWindow::createContainerLayout() {

  const auto layout = new QVBoxLayout(this);
  const auto bottomRow = createBottomRow();
  layout->addWidget(bottomRow);
  return layout;

}

QWidget* FomodInstallerWindow::createBottomRow() {

  // In vanilla FOMOD installer, left has the Manual button, right has back, next/install, and cancel buttons
  const auto bottomRow = new QWidget(this);
  auto* layout = new QHBoxLayout(bottomRow);

  // Manual on far left
  mManualButton = UIHelper::createButton("Manual", bottomRow);
  layout->addWidget(mManualButton);

  // Space to push remaining buttons right
  layout->addStretch();

  mBackButton        = UIHelper::createButton("Back", bottomRow);
  mNextInstallButton = UIHelper::createButton("Next", bottomRow);
  mCancelButton      = UIHelper::createButton("Cancel", bottomRow);

  // TODO: Connect the others.
  connect(mCancelButton, SIGNAL(clicked()), this, SLOT(reject()));

  layout->addWidget(mBackButton);
  layout->addWidget(mNextInstallButton);
  layout->addWidget(mCancelButton);

  bottomRow->setLayout(layout);
  return bottomRow;
}

QWidget* FomodInstallerWindow::renderStep() {
  return nullptr;
}

QWidget* FomodInstallerWindow::renderGroup() {
  return nullptr;
}

QWidget* FomodInstallerWindow::renderPlugin() {
  return nullptr;
}
