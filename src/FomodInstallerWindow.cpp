#include "FomodInstallerWindow.h"
#include "ui/UIHelper.h"

#include <QVBoxLayout>

FomodInstallerWindow::FomodInstallerWindow(InstallerFomodPlus *installer, const GuessedValue<QString> &modName,
  const std::shared_ptr<IFileTree> &tree, std::unique_ptr<ModuleConfiguration> fomodFile,
  std::unique_ptr<FomodInfoFile> infoFile, QWidget *parent): QDialog(parent),
                                                             mInstaller(installer),
                                                             mModName(modName),
                                                             mTree(tree),
                                                             mFomodFile(std::move(fomodFile)),
                                                             mInfoFile(std::move(infoFile)) {

  const auto layout = new QVBoxLayout(this);

  QLabel *label = UIHelper::createLabel("Fomod Installer", this);
  layout->addWidget(label);

  QPushButton *acceptButton = UIHelper::createButton("Accept", this);
  layout->addWidget(acceptButton);

  QPushButton *rejectButton = UIHelper::createButton("Reject", this);
  layout->addWidget(rejectButton);

  connect(acceptButton, SIGNAL(clicked()), this, SLOT(accept()));
  connect(rejectButton, SIGNAL(clicked()), this, SLOT(reject()));

  setLayout(layout);

}
