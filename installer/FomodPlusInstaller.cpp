#include "FomodPlusInstaller.h"

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
#include "stringutil.h"
#include "integration/FomodDataContent.h"
#include "ui/FomodViewModel.h"

bool FomodPlusInstaller::init(IOrganizer* organizer)
{
    mOrganizer = organizer;
    setupUiInjection();
    return true;
}

void FomodPlusInstaller::setupUiInjection() const
{
    if (!mOrganizer) {
        std::cerr << "Organizer is null" << std::endl;
        return;
    }
    const auto fomodContent = std::make_shared<FomodDataContent>(mOrganizer);
    // const auto managedGamePlugin = const_cast<IPluginGame *>(mOrganizer->managedGame());
    mOrganizer->gameFeatures()->registerFeature(fomodContent, 0, false);
}

bool FomodPlusInstaller::isArchiveSupported(std::shared_ptr<const IFileTree> tree) const
{
    tree = findFomodDirectory(tree);
    if (tree != nullptr) {
        return tree->exists(StringConstants::FomodFiles::MODULE_CONFIG.data(), FileTreeEntry::FILE);
    }
    return false;
}

QList<PluginSetting> FomodPlusInstaller::settings() const
{

    return {};
}

nlohmann::json FomodPlusInstaller::getExistingFomodJson(const GuessedValue<QString>& modName) const
{
    auto existingMod = mOrganizer->modList()->getMod(modName);
    if (existingMod == nullptr) {
        for (auto variant : modName.variants()) {
            existingMod = mOrganizer->modList()->getMod(variant);
            if (existingMod != nullptr) {
                break;
            }
        }
    }
    if (existingMod == nullptr) {
        return {};
    }
    const auto fomodJson = existingMod->pluginSetting(name(), "fomod", 0);
    if (fomodJson == 0) {
        return {};
    }
    try {
        return nlohmann::json::parse(fomodJson.toString().toStdString());
    } catch ([[maybe_unused]] Exception e) {
        log::error("Could not parse existing JSON, even though it appears to exist. Returning empty JSON.");
        return {};
    }
}

/**
 *
 * @param modName
 * @param tree
 * @param version
 * @param nexusID
 * @return
 */
IPluginInstaller::EInstallResult FomodPlusInstaller::install(GuessedValue<QString>& modName,
    std::shared_ptr<IFileTree>& tree, QString& version,
    int& nexusID)
{

    log::debug("FomodPlusInstaller::install - modName: {}, version: {}, nexusID: {}",
        modName->toStdString(),
        version.toStdString(),
        nexusID
        );
    log::debug("FomodPlusInstaller::install - tree size: {}", tree->size());

    auto [infoFile, moduleConfigFile] = parseFomodFiles(tree);

    if (infoFile == nullptr || moduleConfigFile == nullptr) {
        return RESULT_FAILED;
    }

    // create ui & pass xml classes to ui
    auto fomodViewModel = FomodViewModel::create(mOrganizer, std::move(moduleConfigFile), std::move(infoFile));
    auto json           = getExistingFomodJson(modName);
    const auto window   = std::make_shared<FomodInstallerWindow>(this, modName, tree, mFomodPath, fomodViewModel, json);

    if (const QDialog::DialogCode result = showInstallerWindow(window); result == QDialog::Accepted) {
        // modname was updated in window
        mInstallerUsed = true;
        const std::shared_ptr<IFileTree> installTree = window->getFileInstaller()->install();
        tree = installTree;
        mFomodJson = std::make_shared<nlohmann::json>(window->getFileInstaller()->generateFomodJson());
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
std::pair<std::unique_ptr<FomodInfoFile>, std::unique_ptr<ModuleConfiguration> > FomodPlusInstaller::parseFomodFiles(
    const std::shared_ptr<IFileTree>& tree)
{
    const auto fomodDir = findFomodDirectory(tree);
    if (fomodDir == nullptr) {
        log::error("FomodPlusInstaller::install - fomod directory not found");
        return { nullptr, nullptr };
    }

    // This is a strange place to set this value but okay for now.
    mFomodPath = fomodDir->parent()->path();

    const auto infoXML = fomodDir->find(
        StringConstants::FomodFiles::INFO_XML.data(),
        FileTreeEntry::FILE
        );
    const auto moduleConfig = fomodDir->find(
        StringConstants::FomodFiles::MODULE_CONFIG.data(),
        FileTreeEntry::FILE
        );

    // Extract files first.
    vector<std::shared_ptr<const FileTreeEntry> > toExtract = {};
    if (moduleConfig) {
        toExtract.push_back(moduleConfig);
    } else {
        log::error("FomodPlusInstaller::install - error parsing moduleConfig.xml: Not Present");
        return { nullptr, nullptr };
    }
    if (infoXML) {
        toExtract.push_back(infoXML);
    }
    appendImageFiles(toExtract, tree);
    const auto paths = manager()->extractFiles(toExtract);

    auto moduleConfiguration = std::make_unique<ModuleConfiguration>();
    try {
        moduleConfiguration->deserialize(paths.at(0).toStdString());
    } catch (XmlParseException& e) {
        log::error("FomodPlusInstaller::install - error parsing moduleConfig.xml: {}", e.what());
        return { nullptr, nullptr };
    }

    auto infoFile = std::make_unique<FomodInfoFile>();
    if (infoXML) {
        try {
            infoFile->deserialize(paths.at(1).toStdString());
        } catch (XmlParseException& e) {
            log::error("FomodPlusInstaller::install - error parsing info.xml: {}", e.what());
        }
    }

    return { std::move(infoFile), std::move(moduleConfiguration) };
}

// Taken from https://github.com/ModOrganizer2/modorganizer-installer_fomod/blob/master/src/installerfomod.cpp#L123
void FomodPlusInstaller::appendImageFiles(vector<shared_ptr<const FileTreeEntry> >& entries,
    const shared_ptr<const IFileTree>& tree)
{
    static std::set<QString, FileNameComparator> imageSuffixes{ "png", "jpg", "jpeg", "gif", "bmp" };
    for (auto entry : *tree) {
        if (entry->isDir()) {
            appendImageFiles(entries, entry->astree());
        } else if (imageSuffixes.contains(entry->suffix())) {
            entries.push_back(entry);
        }
    }
}


void FomodPlusInstaller::onInstallationStart(QString const& archive, const bool reinstallation,
    IModInterface* currentMod)
{
    IPluginInstallerSimple::onInstallationStart(archive, reinstallation, currentMod);
}

void FomodPlusInstaller::onInstallationEnd(const EInstallResult result, IModInterface* newMod)
{
    IPluginInstallerSimple::onInstallationEnd(result, newMod);

    // Update the meta.ini file with the fomod information
    if (mFomodJson != nullptr && result == RESULT_SUCCESS && newMod != nullptr && mInstallerUsed) {
        newMod->setPluginSetting(this->name(), "fomod", mFomodJson->dump().c_str());
    }
}

// Borrowed from https://github.com/ModOrganizer2/modorganizer-installer_fomod/blob/master/src/installerfomod.cpp
std::shared_ptr<const IFileTree> FomodPlusInstaller::findFomodDirectory(const std::shared_ptr<const IFileTree>& tree)
{
    const auto entry = tree->find(StringConstants::FomodFiles::FOMOD_DIR.data(), FileTreeEntry::DIRECTORY);

    if (entry != nullptr) {
        return entry->astree();
    }

    if (tree->size() == 1 && tree->at(0)->isDir()) {
        return findFomodDirectory(tree->at(0)->astree());
    }
    return nullptr;
}

QDialog::DialogCode FomodPlusInstaller::showInstallerWindow(const std::shared_ptr<FomodInstallerWindow>& window)
{
    QEventLoop loop;
    connect(window.get(), SIGNAL(accepted()), &loop, SLOT(quit()));
    connect(window.get(), SIGNAL(rejected()), &loop, SLOT(quit()));
    window->show();
    loop.exec();
    return static_cast<QDialog::DialogCode>(window->result());
}