﻿#include "FomodPlusInstaller.h"

#include <igamefeatures.h>
#include <iinstallationmanager.h>
#include <iplugingame.h>
#include <QEventLoop>
#include <QTreeWidget>
#include <xml/FomodInfoFile.h>
#include <xml/ModuleConfiguration.h>
#include <xml/XmlParseException.h>

#include "FomodInstallerWindow.h"
#include "stringutil.h"
#include "integration/FomodDataContent.h"
#include "ui/Colors.h"
#include "ui/FomodViewModel.h"

#include <QMessageBox>
#include <QSettings>

using namespace Qt::Literals::StringLiterals;

/*
--------------------------------------------------------------------------------
                              Init
--------------------------------------------------------------------------------
*/
#pragma region Initialization

bool FomodPlusInstaller::init(IOrganizer* organizer)
{
    mOrganizer = organizer;
    log.setLogFilePath(QDir::currentPath().toStdString() + "/logs/fomodplus.log");
    std::cout << "QDir::currentPath(): " << QDir::currentPath().toStdString() << std::endl;
    std::cout << "mOrganizer->basePath() : " << mOrganizer->basePath().toStdString() << std::endl;

    // REMEMBER: This mFomodDB persists beyond the scope of an individual install. Do not do anything nasty to it.
    mFomodDb = std::make_unique<FomodDB>(mOrganizer->basePath().toStdString());
    setupUiInjection();
    return true;
}

void FomodPlusInstaller::setupUiInjection() const
{
    const auto fomodContent = std::make_shared<FomodDataContent>(mOrganizer);
    mOrganizer->gameFeatures()->registerFeature(fomodContent, 0, false);
}

#pragma endregion

/*
--------------------------------------------------------------------------------
                              Settings
--------------------------------------------------------------------------------
*/
#pragma region Settings
bool FomodPlusInstaller::shouldFallbackToLegacyInstaller() const
{
    return mOrganizer->pluginSetting(name(), "fallback_to_legacy").value<bool>();
}

bool FomodPlusInstaller::shouldShowImages() const
{
    return mOrganizer->pluginSetting(name(), "show_images").value<bool>();
}

bool FomodPlusInstaller::shouldShowNotifications() const
{
    return mOrganizer->pluginSetting(name(), "show_notifications").value<bool>();
}

bool FomodPlusInstaller::shouldAutoRestoreChoices() const
{
    return mOrganizer->pluginSetting(name(), "always_restore_choices").value<bool>();
}

bool FomodPlusInstaller::isWizardIntegrated() const
{
    return mOrganizer->pluginSetting(name(), "wizard_integration").value<bool>();
}

void FomodPlusInstaller::toggleShouldShowImages() const
{
    const bool showImages = shouldShowImages();
    mOrganizer->setPluginSetting(name(), "show_images", !showImages);
}

QString FomodPlusInstaller::getSelectedColor() const
{
    const auto colorName = mOrganizer->pluginSetting(name(), "color_theme").toString();
    const auto it        = UiColors::colorStyles.find(colorName);
    return it != UiColors::colorStyles.end() ? it->first : "Blue";
}

std::vector<std::shared_ptr<const IPluginRequirement> > FomodPlusInstaller::requirements() const
{
    return { Requirements::gameDependency(
    { u"Oblivion"_s, u"Fallout 3"_s, u"New Vegas"_s, u"Skyrim"_s, u"Enderal"_s,
      u"Fallout 4"_s, u"Skyrim Special Edition"_s, u"Enderal Special Edition"_s,
      u"Skyrim VR"_s, u"Fallout 4 VR"_s, u"Starfield"_s }) };
}

QList<PluginSetting> FomodPlusInstaller::settings() const
{
    return {
        { u"fallback_to_legacy"_s, u"When hitting cancel, fall back to the legacy FOMOD installer."_s, false },
        { u"always_restore_choices"_s, u"Restore previous choices without clicking the magic button"_s, false },
        { u"show_images"_s, u"Show image previews and the image carousel in installer windows."_s, true },
        { u"color_theme"_s, u"Select the color theme for the installer"_s, QString("Blue") }, // Default color name
        { u"show_notifications"_s, u"Show the notifications panel"_s, false }, //WIP
        { u"wizard_integration"_s, u"Integrate the installer with patch wizard."_s, true }, //WIP
    };
}

#pragma endregion

bool FomodPlusInstaller::isArchiveSupported(std::shared_ptr<const IFileTree> tree) const
{
    tree = findFomodDirectory(tree);
    if (tree != nullptr) {
        return tree->exists(StringConstants::FomodFiles::MODULE_CONFIG.data(), FileTreeEntry::FILE);
    }
    return false;
}


nlohmann::json FomodPlusInstaller::getExistingFomodJson(const GuessedValue<QString>& modName) const
{
    auto existingMod = mOrganizer->modList()->getMod(modName);
    if (existingMod == nullptr) {
        for (const auto& variant : modName.variants()) {
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
    } catch ([[maybe_unused]] Exception& e) {
        logMessage(ERR, "Could not parse existing JSON, even though it appears to exist. Returning empty JSON.");
        return {};
    }
}

void FomodPlusInstaller::clearPriorInstallData()
{
    mNotes         = "";
    mInstallerUsed = false;
    mFomodJson     = nullptr;
    mFomodPath     = "";
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
    clearPriorInstallData();

    logMessage(INFO, std::format("FomodPlusInstaller::install - modName: {}, version: {}, nexusID: {}",
        modName->toStdString(),
        version.toStdString(),
        nexusID
        ));
    logMessage(INFO, std::format("FomodPlusInstaller::install - tree size: {}", tree->size()));

    auto [infoFile, moduleConfigFile, filePaths] = parseFomodFiles(tree);
    std::vector<QString> pluginPaths = {};
    for (const auto& filePath : filePaths) {
        if (isPluginFile(filePath)) {
            pluginPaths.emplace_back(filePath);
        }
    }

    const auto dbEntry = mFomodDb->getEntryFromFomod(moduleConfigFile.get(), pluginPaths, nexusID);

    if (infoFile == nullptr || moduleConfigFile == nullptr) {
        return RESULT_FAILED;
    }

    // create ui & pass xml classes to ui
    auto fomodViewModel = FomodViewModel::create(mOrganizer, std::move(moduleConfigFile), std::move(infoFile));
    auto json           = getExistingFomodJson(modName);
    const auto window   = std::make_shared<FomodInstallerWindow>(this, modName, tree, mFomodPath, fomodViewModel, json);

    const QDialog::DialogCode result = showInstallerWindow(window);
    if (result == QDialog::Accepted) {
        // modname was updated in window
        mInstallerUsed = true;
        const std::shared_ptr<IFileTree> installTree = window->getFileInstaller()->install();
        tree = installTree;
        mFomodJson = std::make_shared<nlohmann::json>(window->getFileInstaller()->generateFomodJson());
        mNotes = window->getFileInstaller()->createInstallationNotes();

        try {
            mFomodDb->addEntry(dbEntry);
            mFomodDb->saveToFile();
        } catch ([[maybe_unused]] Exception& e) {
            logMessage(ERR, "Failed to add FomodDB entries.");
            logMessage(ERR, e.what());
        }
        return RESULT_SUCCESS;
    }
    if (window->isManualInstall()) {
        return RESULT_MANUALREQUESTED;
    }
    if (shouldFallbackToLegacyInstaller()) {
        return RESULT_NOTATTEMPTED;
    }
    return RESULT_CANCELED;
}


/**
 *
 * @param tree
 * @return
 */
ParsedFilesTuple FomodPlusInstaller::parseFomodFiles(const std::shared_ptr<IFileTree>& tree)
{
    const auto emptyResult = std::make_tuple(nullptr, nullptr, QStringList());

    const auto fomodDir = findFomodDirectory(tree);
    if (fomodDir == nullptr) {
        logMessage(ERR, "FomodPlusInstaller::install - fomod directory not found");
        return emptyResult;
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
        logMessage(ERR, "FomodPlusInstaller::install - error parsing moduleConfig.xml: Not Present");
        return emptyResult;
    }
    if (infoXML) {
        toExtract.push_back(infoXML);
    }
    appendImageFiles(toExtract, tree);
    appendPluginFiles(toExtract, tree); // For patch wizard data collection
    const auto paths = manager()->extractFiles(toExtract);

    auto moduleConfiguration = std::make_unique<ModuleConfiguration>();
    try {
        moduleConfiguration->deserialize(paths.at(0));
    } catch (XmlParseException& e) {
        logMessage(ERR, std::format("FomodPlusInstaller::install - error parsing moduleConfig.xml: {}", e.what()));
        return emptyResult;
    }

    auto infoFile = std::make_unique<FomodInfoFile>();
    if (infoXML) {
        try {
            infoFile->deserialize(paths.at(1));
        } catch (XmlParseException& e) {
            logMessage(ERR, std::format("FomodPlusInstaller::install - error parsing info.xml: {}", e.what()));
        }
    }

    return std::make_tuple(std::move(infoFile), std::move(moduleConfiguration), paths);
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

void FomodPlusInstaller::appendPluginFiles(vector<shared_ptr<const FileTreeEntry> >& entries,
    const shared_ptr<const IFileTree>& tree)
{
    // Don't bother with this stuff if the user doesn't care about the wizard.
    // It may slightly bloat the temp file directory if not needed.
    if (isWizardIntegrated()) {
        for (auto entry : *tree) {
            if (entry->isDir()) {
                appendPluginFiles(entries, entry->astree());
            } else if (isPluginFile(entry->suffix())) {
                entries.push_back(entry);
            }
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
        // writeNotes(newMod);
        mOrganizer->refresh();
    }
    clearPriorInstallData();
}

[[deprecated]]
void FomodPlusInstaller::writeNotes(IModInterface* newMod) const
{
    if (mNotes.isEmpty()) {
        return;
    }

    const auto iniKey = "installationNotes";
    newMod->setPluginSetting(this->name(), iniKey, mNotes);
    logMessage(INFO, "Wrote notes to meta.ini. Needs handler for adding to actual meta.ini 'notes' field.");
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

void showError(const Exception& e)
{
    const QString errorText = "Mod Name: \n" "Nexus ID: " "\n" "Exception: " + QString(e.what()) + "\n";

    QMessageBox msgBox;
    msgBox.setWindowTitle("FOMOD Plus Error :(");
    msgBox.setTextFormat(Qt::RichText);
    msgBox.setText("Sorry this happened. Please copy the following error and report it to me on Nexus or GitHub.\n"
        "<pre style='background-color: #f0f0f0; padding: 10px;'>"
        + errorText +
        "</pre>"
        );
    msgBox.setIcon(QMessageBox::Critical);
    msgBox.exec();

}