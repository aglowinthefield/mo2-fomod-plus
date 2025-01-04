#include "InstallerFomodPlus.h"

#include <iinstallationmanager.h>
#include <log.h>
#include <QEventLoop>
#include <xml/FomodInfoFile.h>
#include <xml/ModuleConfiguration.h>

#include "FomodInstallerWindow.h"
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

/**
 *
 * @param modName
 * @param tree
 * @param version
 * @param nexusID
 * @return
 */
IPluginInstaller::EInstallResult InstallerFomodPlus::install(GuessedValue<QString> &modName,
  std::shared_ptr<IFileTree> &tree, QString &version, int &nexusID) {

  log::debug("InstallerFomodPlus::install - modName: {}, version: {}, nexusID: {}",
             modName->toStdString(),
             version.toStdString(),
             nexusID
  );
  log::debug("InstallerFomodPlus::install - tree size: {}", tree->size());

  auto [infoFile, moduleConfigFile] = parseFomodFiles(tree);

  if (infoFile == nullptr || moduleConfigFile == nullptr) {
    // Do we want to fail if no info.xml? probably for now. something to consider here.
    return RESULT_FAILED;
  }
  // create ui & pass xml classes to ui
  const auto window = std::make_shared<FomodInstallerWindow>(this, modName, tree, std::move(moduleConfigFile), std::move(infoFile));

  const QDialog::DialogCode result = showInstallerWindow(window);
  if (result == QDialog::Accepted) {
    return RESULT_SUCCESS;
  }
  return RESULT_NOTATTEMPTED;
}

/**
 *
 * @param tree
 * @return
 */
std::pair<std::unique_ptr<FomodInfoFile>, std::unique_ptr<ModuleConfiguration>> InstallerFomodPlus::parseFomodFiles(
  const std::shared_ptr<IFileTree> &tree) const {
  const auto fomodDir = findFomodDirectory(tree);
  if (fomodDir == nullptr) {
    log::error("InstallerFomodPlus::install - fomod directory not found");
    return {nullptr, nullptr};
  }

  const auto infoXML = fomodDir->find(
    StringConstants::FomodFiles::INFO_XML,
    FileTreeEntry::FILE
    );
  const auto moduleConfig = fomodDir->find(
    StringConstants::FomodFiles::MODULE_CONFIG,
    FileTreeEntry::FILE
    );

  // Extract files first.
  auto paths = manager()->extractFiles({infoXML, moduleConfig});

  // parse xml
  auto infoFile = std::make_unique<FomodInfoFile>();
  if (!infoFile->deserialize(paths.first().toStdString())) {
    log::debug("Could not deserialize info file. See logs for more information.");
    return {nullptr, nullptr};
  }

  auto moduleConfiguration = std::make_unique<ModuleConfiguration>();
  if (!moduleConfiguration->deserialize(paths.last().toStdString())) {
    log::debug("Could not deserialize ModuleConfig file. See logs for more information.");
    return {nullptr, nullptr};
  }

  return {std::move(infoFile), std::move(moduleConfiguration)};

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

QDialog::DialogCode InstallerFomodPlus::showInstallerWindow(const std::shared_ptr<FomodInstallerWindow>& window) {
  log::debug("InstallerFomodPlus::showInstallerWindow - entering function");
  QEventLoop loop;
  connect(window.get(), SIGNAL(accepted()), &loop, SLOT(quit()));
  connect(window.get(), SIGNAL(rejected()), &loop, SLOT(quit()));
  log::debug("InstallerFomodPlus::showInstallerWindow - starting event loop");
  window->show();
  loop.exec();
  log::debug("InstallerFomodPlus::showInstallerWindow - event loop finished");
  return static_cast<QDialog::DialogCode>(window->result());
}

