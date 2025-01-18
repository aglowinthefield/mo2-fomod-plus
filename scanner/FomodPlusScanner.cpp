#include "FomodPlusScanner.h"

#include <QDialog>
#include <QPushButton>
#include <QVBoxLayout>
#include <archive.h>

#include <iostream>

bool FomodPlusScanner::init(IOrganizer* organizer) {
  mOrganizer = organizer;
  return true;
}

void FomodPlusScanner::display() const {
  QDialog dialog;
  dialog.setWindowTitle("FOMOD Scanner");

  QVBoxLayout layout(&dialog);

  QPushButton scanButton("Scan", &dialog);
  QPushButton cancelButton("Cancel", &dialog);

  layout.addWidget(&scanButton);
  layout.addWidget(&cancelButton);

  connect(&cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);
  connect(&scanButton, &QPushButton::clicked, [this, &dialog] {
    this->scanLoadOrder();
    dialog.accept();
  });

  dialog.exec();
}

void FomodPlusScanner::scanLoadOrder() const {
  const auto pluginName = "FOMOD Plus"; // TODO: can this be read from shared constants?

  for (auto modName : mOrganizer->modList()->allMods()) {
    const auto mod = mOrganizer->modList()->getMod(modName);
    if (auto setting = mod->pluginSetting(pluginName, "fomod", 0); setting == 0) {
      continue; // We've already set metadata for this FOMOD.
    }

    // check archives
    int result = openInstallationArchive(mod);
    std::cout << "Result is " << result << std::endl;


  }
}

int FomodPlusScanner::openInstallationArchive(const IModInterface* mod) const {
  const auto downloadsDir = mOrganizer->downloadsPath();
  const auto installationFilePath = mod->installationFile();
  const auto qualifiedInstallerPath = downloadsDir + "/" + installationFilePath;

  const auto archive = CreateArchive();

  if (!archive->isValid()) {
    std::wcerr << "Failed to load the archive module: " << std::endl;
    return -1;
  }

  // Open the archive:
  if (!archive->open(qualifiedInstallerPath.toStdWString(), nullptr)) {
    std::wcerr << "Failed to open the archive: " << std::endl;
    return -1;
  }

  // Get the list of files:

  // Mark all files for extraction to their path in the archive:
  for (auto const& files = archive->getFileList(); const auto *fileData: files) {
    std::wcout << fileData->getArchiveFilePath() << std::endl;
  }
  return 0;
}

bool FomodPlusScanner::setFomodInfoForMod(IModInterface* mod) {
  const auto pluginName = "FOMOD Plus"; // TODO: can this be read from shared constants?
  return mod->setPluginSetting(pluginName, "fomod", "{}");
}