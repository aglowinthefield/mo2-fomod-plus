#include "FomodPlus.h"

#include <igamefeatures.h>
#include <iinstallationmanager.h>
#include <iplugingame.h>
#include <log.h>
#include <QEventLoop>
#include <QTreeWidget>
#include <xml/FomodInfoFile.h>
#include <xml/ModuleConfiguration.h>
#include <xml/XmlParseException.h>

#include "FomodInstallerWindow.h"
#include "integration/FomodDataContent.h"
#include "lib/stringutil.h"
#include "ui/FomodViewModel.h"

bool FomodPlus::init(IOrganizer *organizer) {
  mOrganizer = organizer;
  setupUiInjection();
  return true;
}

void FomodPlus::setupUiInjection() const {
  if (!mOrganizer) {
    std::cerr << "Organizer is null" << std::endl;
    return;
  }
  const auto fomodContent = std::make_shared<FomodDataContent>(mOrganizer);
  // const auto managedGamePlugin = const_cast<IPluginGame *>(mOrganizer->managedGame());
  mOrganizer->gameFeatures()->registerFeature(fomodContent, 0, false);
}

bool FomodPlus::isArchiveSupported(std::shared_ptr<const IFileTree> tree) const {
  tree = findFomodDirectory(tree);
  if (tree != nullptr) {
    return tree->exists(StringConstants::FomodFiles::MODULE_CONFIG, FileTreeEntry::FILE);
  }
  return false;
}

QList<PluginSetting> FomodPlus::settings() const {
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
IPluginInstaller::EInstallResult FomodPlus::install(GuessedValue<QString> &modName,
                                                             std::shared_ptr<IFileTree> &tree, QString &version,
                                                             int &nexusID) {
  log::debug("FomodPlus::install - modName: {}, version: {}, nexusID: {}",
             modName->toStdString(),
             version.toStdString(),
             nexusID
  );
  log::debug("FomodPlus::install - tree size: {}", tree->size());

  auto [infoFile, moduleConfigFile] = parseFomodFiles(tree);

  if (infoFile == nullptr || moduleConfigFile == nullptr) {
    // Do we want to fail if no info.xml? probably for now. something to consider here.
    return RESULT_FAILED;
  }

  // create ui & pass xml classes to ui
  auto fomodViewModel = FomodViewModel::create(mOrganizer, std::move(moduleConfigFile), std::move(infoFile));
  const auto window = std::make_shared<FomodInstallerWindow>(
    this,
    modName,
    tree,
    mFomodPath,
    fomodViewModel
  );

  if (const QDialog::DialogCode result = showInstallerWindow(window); result == QDialog::Accepted) {
    // modname was updated in window
    const std::shared_ptr<IFileTree> installTree = window->getFileInstaller()->install();
    tree = installTree;
    mFomodJson = std::make_shared<nlohmann::json>(window->getFileInstaller()->generateFomodJson());

    // if (const auto entry = tree->find("fomod.json", FileTreeEntry::FILE); entry != nullptr) {
      // const auto fomodJsonFilePath = manager()->createFile(entry);
      // window->getFileInstaller()->writeFomodJsonToFile(fomodJsonFilePath.toStdString());
    // }

    return RESULT_SUCCESS;
  }
  if (window->isManualInstall()) {
    return RESULT_MANUALREQUESTED;
  }
  return RESULT_NOTATTEMPTED;
}

/**
 *
 * @param tree
 * @return
 */
std::pair<std::unique_ptr<FomodInfoFile>, std::unique_ptr<ModuleConfiguration> > FomodPlus::parseFomodFiles(
  const std::shared_ptr<IFileTree> &tree) {
  const auto fomodDir = findFomodDirectory(tree);
  if (fomodDir == nullptr) {
    log::error("FomodPlus::install - fomod directory not found");
    return {nullptr, nullptr};
  }

  // This is a strange place to set this value but okay for now.
  mFomodPath = fomodDir->parent()->path();

  const auto infoXML = fomodDir->find(
    StringConstants::FomodFiles::INFO_XML,
    FileTreeEntry::FILE
  );
  const auto moduleConfig = fomodDir->find(
    StringConstants::FomodFiles::MODULE_CONFIG,
    FileTreeEntry::FILE
  );

  // Extract files first.
  vector toExtract = {infoXML, moduleConfig};
  appendImageFiles(toExtract, tree);
  const auto paths = manager()->extractFiles(toExtract);

  auto infoFile = std::make_unique<FomodInfoFile>();
  try {
    infoFile->deserialize(paths.at(0).toStdString());
  } catch (XmlParseException &e) {
    log::error("FomodPlus::install - error parsing info.xml: {}", e.what());
    return {nullptr, nullptr};
  }

  auto moduleConfiguration = std::make_unique<ModuleConfiguration>();
  try {
    moduleConfiguration->deserialize(paths.at(1).toStdString());
  } catch (XmlParseException &e) {
    log::error("FomodPlus::install - error parsing moduleConfig.xml: {}", e.what());
    return {nullptr, nullptr};
  }
  return {std::move(infoFile), std::move(moduleConfiguration)};
}

// Taken from https://github.com/ModOrganizer2/modorganizer-installer_fomod/blob/master/src/installerfomod.cpp#L123
void FomodPlus::appendImageFiles(vector<shared_ptr<const FileTreeEntry> > &entries,
                                          const shared_ptr<const IFileTree> &tree) {
  static std::set<QString, FileNameComparator> imageSuffixes{"png", "jpg", "jpeg", "gif", "bmp"};
  for (auto entry: *tree) {
    if (entry->isDir()) {
      appendImageFiles(entries, entry->astree());
    } else if (imageSuffixes.contains(entry->suffix())) {
      entries.push_back(entry);
    }
  }
}


void FomodPlus::onInstallationStart(QString const &archive, const bool reinstallation,
                                             IModInterface *currentMod) {
  IPluginInstallerSimple::onInstallationStart(archive, reinstallation, currentMod);
}

void FomodPlus::onInstallationEnd(const EInstallResult result, IModInterface *newMod) {
  IPluginInstallerSimple::onInstallationEnd(result, newMod);

  // Update the meta.ini file with the fomod information
  if (mFomodJson != nullptr) {
    newMod->setPluginSetting(this->name(), "fomod", mFomodJson->dump().c_str());
  }
}

// Borrowed from https://github.com/ModOrganizer2/modorganizer-installer_fomod/blob/master/src/installerfomod.cpp
std::shared_ptr<const IFileTree> FomodPlus::findFomodDirectory(const std::shared_ptr<const IFileTree> &tree) {
  const auto entry = tree->find(StringConstants::FomodFiles::FOMOD_DIR, FileTreeEntry::DIRECTORY);

  if (entry != nullptr) {
    return entry->astree();
  }

  if (tree->size() == 1 && tree->at(0)->isDir()) {
    return findFomodDirectory(tree->at(0)->astree());
  }
  return nullptr;
}

QDialog::DialogCode FomodPlus::showInstallerWindow(const std::shared_ptr<FomodInstallerWindow> &window) {
  QEventLoop loop;
  connect(window.get(), SIGNAL(accepted()), &loop, SLOT(quit()));
  connect(window.get(), SIGNAL(rejected()), &loop, SLOT(quit()));
  window->show();
  loop.exec();
  return static_cast<QDialog::DialogCode>(window->result());
}
