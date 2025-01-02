#include <log.h>

#include "InstallerFomodPlus.h"
#include "stringconstants.h"

bool InstallerFomodPlus::init(IOrganizer *organizer) {
  m_Organizer = organizer;
  return true;
}

bool InstallerFomodPlus::isArchiveSupported(std::shared_ptr<const IFileTree> tree) const {
  tree = findFomodDirectory(tree);
  if (tree != nullptr) {
    return tree->exists(StringConstants::FomodFiles::MODULE_CONFIG, FileTreeEntry::FILE);
  }
  return false;
}

QList<PluginSetting> InstallerFomodPlus::settings() const {
  return {};
}

IPluginInstaller::EInstallResult InstallerFomodPlus::install(GuessedValue<QString> &modName,
  std::shared_ptr<IFileTree> &tree, QString &version, int &nexusID) {

  log::debug("InstallerFomodPlus::install - modName: {}, version: {}, nexusID: {}",
             modName->toStdString(),
             version.toStdString(),
             nexusID
  );

  log::debug("InstallerFomodPlus::install - tree size: {}", tree->size());

  return RESULT_NOTATTEMPTED;
}

void InstallerFomodPlus::onInstallationStart(QString const &archive, bool reinstallation,
  IModInterface *currentMod) {
  IPluginInstallerSimple::onInstallationStart(archive, reinstallation, currentMod);
}

void InstallerFomodPlus::onInstallationEnd(EInstallResult result, IModInterface *newMod) {
  IPluginInstallerSimple::onInstallationEnd(result, newMod);
}

// Borrowed from https://github.com/ModOrganizer2/modorganizer-installer_fomod/blob/master/src/installerfomod.cpp
std::shared_ptr<const IFileTree> InstallerFomodPlus::findFomodDirectory(const std::shared_ptr<const IFileTree> &tree) {
  const auto entry = tree->find(StringConstants::FomodFiles::FOMOD_DIR, FileTreeEntry::DIRECTORY);

  if (entry != nullptr) {
    return entry->astree();
  }

  if (tree->size() == 1 && tree->at(0)->isDir()) {
    return findFomodDirectory(tree->at(0)->astree());
  }
  return nullptr;
}
