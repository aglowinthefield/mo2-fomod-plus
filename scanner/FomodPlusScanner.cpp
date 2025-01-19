#include "FomodPlusScanner.h"

#include <QDialog>
#include <QPushButton>
#include <QProgressBar>
#include <QVBoxLayout>
#include <QMessageBox>
#include <archive.h>
#include <string>

#include <QLabel>
#include <iostream>

#include "stringutil.h"

bool FomodPlusScanner::init(IOrganizer* organizer) {
  mOrganizer = organizer;

  mDialog = new QDialog();
  mDialog->setWindowTitle("FOMOD Scanner");

  const auto layout = new QVBoxLayout(mDialog);

  std::string   wizard      = "";
  const QString description = "Greetings, traveler.\n"
      "This tool will scan your load order for mods installed via FOMOD.\n"
      "It might take a minute to complete once you hit 'Scan', so please be patient!\n\n"
      "Safe travels, and may your load order be free of conflicts.";

  const auto descriptionLabel = new QLabel(description, mDialog);
  descriptionLabel->setWordWrap(true); // Enable word wrap for large text
  descriptionLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  descriptionLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop); // Align text to the top-left corner

  mProgressBar = new QProgressBar(mDialog);
  mProgressBar->setRange(0, mOrganizer->modList()->allMods().size());
  mProgressBar->setVisible(false);

  const auto scanButton = new QPushButton("Scan", mDialog);
  const auto cancelButton = new QPushButton("Cancel", mDialog);

  layout->addWidget(descriptionLabel, 1);
  layout->addWidget(mProgressBar, 1);
  layout->addWidget(scanButton, 1);
  layout->addWidget(cancelButton, 1);

  connect(cancelButton, &QPushButton::clicked, mDialog, &QDialog::reject);
  connect(scanButton, &QPushButton::clicked, [this] {
    mProgressBar->setVisible(true);
    const int added = this->scanLoadOrder();
    this->mDialog->accept();
    QMessageBox::information(mDialog, "Scan Complete", "The load order scan is complete. Added filter info to " + QString::number(added) + " mods.");

  });
  connect(mDialog, &QDialog::finished, this, &FomodPlusScanner::cleanup);

  mDialog->setLayout(layout);
  mDialog->setMinimumSize(400, 300);
  mDialog->adjustSize();
  descriptionLabel->adjustSize();
  return true;
}

void FomodPlusScanner::cleanup() const {
  mProgressBar->reset();
  mProgressBar->setVisible(false);
}

void FomodPlusScanner::display() const {
  mDialog->exec();
}

int FomodPlusScanner::scanLoadOrder() const {
  int progress = 0;
  int added = 0;
  for (auto modName : mOrganizer->modList()->allMods()) {
    const auto pluginName = "FOMOD Plus";
    const auto mod        = mOrganizer->modList()->getMod(modName);
    if (auto setting = mod->pluginSetting(pluginName, "fomod", 0); setting == 0) {
      if (const int result = openInstallationArchive(mod); result == 0) {
        setFomodInfoForMod(mod);
        added++;
      }
    }
    mProgressBar->setValue(++progress);
  }
  return added;
}

bool hasFomodFiles(const std::vector<FileData*>& files) {

  bool hasModuleXml = false;
  bool hasInfoXml = false;

  for (const auto* file : files) {
    if (endsWithCaseInsensitive(file->getArchiveFilePath(), StringConstants::FomodFiles::W_MODULE_CONFIG.data())) {
      hasModuleXml = true;
    }
    if (endsWithCaseInsensitive(file->getArchiveFilePath(), StringConstants::FomodFiles::W_INFO_XML.data())) {
      hasInfoXml = true;
    }
  }
  return hasModuleXml && hasInfoXml;
}

std::ostream& operator<<(std::ostream& os, const Archive::Error& error) {
  switch (error) {
    case Archive::Error::ERROR_NONE:
      os << "No error";
    break;
    case Archive::Error::ERROR_ARCHIVE_NOT_FOUND:
      os << "File not found";
    break;
    case Archive::Error::ERROR_FAILED_TO_OPEN_ARCHIVE:
      os << "Failed to open file";
    break;
    case Archive::Error::ERROR_INVALID_ARCHIVE_FORMAT:
      os << "Invalid archive format";
    break;
    default:
      os << "Unknown error??";
  }
  return os;
}

int FomodPlusScanner::openInstallationArchive(const IModInterface* mod) const {
  const auto downloadsDir = mOrganizer->downloadsPath();
  const auto installationFilePath = mod->installationFile();

  if (installationFilePath.isEmpty()) {
    return -1;
  }

  const auto qualifiedInstallerPath = downloadsDir + "/" + installationFilePath;

  const auto archive = CreateArchive();

  if (!archive->isValid()) {
    std::cerr << "[" << mod->name().toStdString() << "] Failed to load the archive module: " << archive->getLastError() << std::endl;
    return -1;
  }

  // Open the archive:
  if (!archive->open(qualifiedInstallerPath.toStdWString(), nullptr)) {
    std::cerr << "[" << mod->name().toStdString() << "] Failed to open the archive [" << qualifiedInstallerPath.toStdString() <<  "]: " << archive->getLastError() << std::endl;
    return -1;
  }

  // Mark all files for extraction to their path in the archive:
  if (hasFomodFiles(archive->getFileList())) {
    std::cout << "Found FOMOD files in " << qualifiedInstallerPath.toStdString() << std::endl;
    return 0;
  }

  return -1;
}

bool FomodPlusScanner::setFomodInfoForMod(IModInterface* mod) {
  const auto pluginName = "FOMOD Plus"; // TODO: can this be read from shared constants?
  return mod->setPluginSetting(pluginName, "fomod", "{}");
}