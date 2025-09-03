#include "FomodPlusInstaller.h"

#include "xml/FomodInfoFile.h"
#include "xml/ModuleConfiguration.h"
#include "xml/XmlParseException.h"
#include <QEventLoop>
#include <QTreeWidget>
#include <uibase/game_features/igamefeatures.h>
#include <uibase/iinstallationmanager.h>
#include <uibase/iplugingame.h>

#include "FomodInstallerWindow.h"
#include "integration/FomodDataContent.h"
#include "lib/CrashHandler.h"
#include "ui/Colors.h"
#include "ui/FomodViewModel.h"
#include <fomod_plus_shared/stringutil.h>

#include <QMessageBox>
#include <QSettings>

using namespace Qt::Literals::StringLiterals;

bool FomodPlusInstaller::init(IOrganizer* organizer)
{
    CrashHandler::initialize();
    mOrganizer    = organizer;
    mFomodContent = make_shared<FomodDataContent>(organizer);
    log.setLogFilePath(QDir::currentPath().toStdString() + "/logs/fomodplus.log");
    setupUiInjection();
    return true;
}

void FomodPlusInstaller::setupUiInjection() const
{
    if (shouldShowSidebarFilter()) {
        mOrganizer->gameFeatures()->registerFeature(mFomodContent, 9999, false);
    }

    mOrganizer->onPluginSettingChanged(
        [this](const QString& pluginName, const QString& key, const QVariant& oldValue, const QVariant& newValue) {
            if (pluginName == name() && key == "show_fomod_filter") {
                toggleFeature(newValue.toBool());
            }
        });
}

void FomodPlusInstaller::toggleFeature(const bool enabled) const
{
    if (enabled) {
        mOrganizer->gameFeatures()->registerFeature(mFomodContent, 9999, false);
    } else {
        mOrganizer->gameFeatures()->unregisterFeature(mFomodContent);
    }
    mOrganizer->refresh();
}

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

bool FomodPlusInstaller::shouldShowSidebarFilter() const
{
    return mOrganizer->pluginSetting(name(), "show_fomod_filter").value<bool>();
}

bool FomodPlusInstaller::shouldAutoRestoreChoices() const
{
    return mOrganizer->pluginSetting(name(), "always_restore_choices").value<bool>();
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

std::vector<std::shared_ptr<const IPluginRequirement>> FomodPlusInstaller::requirements() const
{
    return { Requirements::gameDependency({ u"Morrowind"_s, u"Oblivion"_s, u"Fallout 3"_s, u"New Vegas"_s, u"Skyrim"_s,
        u"Enderal"_s, u"Fallout 4"_s, u"Skyrim Special Edition"_s, u"Enderal Special Edition"_s, u"Skyrim VR"_s,
        u"Fallout 4 VR"_s, u"Starfield"_s }) };
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
    return { { u"fallback_to_legacy"_s, u"When hitting cancel, fall back to the legacy FOMOD installer."_s, false },
        { u"always_restore_choices"_s, u"Restore previous choices without clicking the magic button"_s, true },
        { u"show_images"_s, u"Show image previews and the image carousel in installer windows."_s, true },
        { u"color_theme"_s, u"Select the color theme for the installer"_s, QString("Blue") }, // Default color name
        { u"show_notifications"_s, u"Show the notifications panel"_s, false },
        { u"show_fomod_filter"_s, u"Show the filter in the sidebar (may break other content filters)"_s, true } };
}

std::pair<nlohmann::json, IModInterface*> FomodPlusInstaller::getExistingFomodJson(
    const GuessedValue<QString>& modName, const int& nexusId, const int& stepsInCurrentFomod) const
{
    logMessage(DEBUG,
        "FomodPlusInstaller::getExistingFomodJson - modName: " + modName->toStdString()
            + " nexusId: " + std::to_string(nexusId));

    // Need to have better mod matching based on presence of FOMOD plugin data.
    struct ModMatch {
        MOBase::IModInterface* mod;
        bool hasFomodData;
        int stepCount;
        nlohmann::json fomodJson;
    };
    std::vector<ModMatch> matches;

    auto parseStepCount = [](const QVariant& fomodData) -> std::pair<bool, int> {
        try {
            if (fomodData.isValid() && !fomodData.isNull()) {
                if (auto json = nlohmann::json::parse(fomodData.toString().toStdString());
                    json.contains("steps") && json["steps"].is_array()) {
                    return { true, static_cast<int>(json["steps"].size()) };
                }
            }
        } catch (...) {
            // Ignore parsing errors
        }
        return { false, 0 };
    };

    // Check exact match first
    if (const auto exactMod = mOrganizer->modList()->getMod(modName)) {
        const auto fomodData = exactMod->pluginSetting(name(), "fomod", 0);
        if (auto [valid, stepCount] = parseStepCount(fomodData); valid) {
            matches.push_back({ exactMod, true, stepCount, nlohmann::json::parse(fomodData.toString().toStdString()) });
        } else {
            matches.push_back({ exactMod, false, 0, nlohmann::json() });
        }
    }

    // Check all variants
    for (const auto& variant : modName.variants()) {
        if (const auto variantMod = mOrganizer->modList()->getMod(variant)) {
            const auto fomodData = variantMod->pluginSetting(name(), "fomod", 0);
            if (auto [valid, stepCount] = parseStepCount(fomodData); valid) {
                matches.push_back(
                    { variantMod, true, stepCount, nlohmann::json::parse(fomodData.toString().toStdString()) });
            } else {
                matches.push_back({ variantMod, false, 0, nlohmann::json() });
            }
        }
    }

    // No matches found
    if (matches.empty()) {
        return {};
    }

    // First try to find exact step count match
    const auto exactMatch = ranges::find_if(matches, [stepsInCurrentFomod](const ModMatch& match) {
        return match.hasFomodData && match.stepCount == stepsInCurrentFomod;
    });

    if (exactMatch != matches.end()) {
        logMessage(DEBUG,
            "Found exact step count match in mod: " + exactMatch->mod->name().toStdString() + " with "
                + std::to_string(exactMatch->stepCount) + " steps");
        return std::make_pair(exactMatch->fomodJson, exactMatch->mod);
    }

    // Find the closest step count among mods with FOMOD data
    const auto closestMatch = ranges::min_element(matches, [stepsInCurrentFomod](const ModMatch& a, const ModMatch& b) {
        if (!a.hasFomodData || !b.hasFomodData)
            return false;
        // The min difference between the step counts
        return std::abs(a.stepCount - stepsInCurrentFomod) < std::abs(b.stepCount - stepsInCurrentFomod);
    });

    if (closestMatch != matches.end() && closestMatch->hasFomodData) {
        logMessage(DEBUG,
            "Using closest step count match from mod: " + closestMatch->mod->name().toStdString() + " with "
                + std::to_string(closestMatch->stepCount) + " steps");
        return std::make_pair(closestMatch->fomodJson, closestMatch->mod);
    }

    // Fallback to first mod with any FOMOD data
    const auto anyFomod = ranges::find_if(matches, [](const ModMatch& match) { return match.hasFomodData; });

    if (anyFomod != matches.end()) {
        logMessage(DEBUG, "Using first available FOMOD data from mod: " + anyFomod->mod->name().toStdString());
        return std::make_pair(anyFomod->fomodJson, anyFomod->mod);
    }

    logMessage(DEBUG, "No matching FOMOD data found");
    return {};
}

void FomodPlusInstaller::clearPriorInstallData()
{
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
IPluginInstaller::EInstallResult FomodPlusInstaller::install(
    GuessedValue<QString>& modName, std::shared_ptr<IFileTree>& tree, QString& version, int& nexusID)
{

    clearPriorInstallData();

    logMessage(INFO,
        std::format("FomodPlusInstaller::install - modName: {}, version: {}, nexusID: {}", modName->toStdString(),
            version.toStdString(), nexusID));
    logMessage(INFO, std::format("FomodPlusInstaller::install - tree size: {}", tree->size()));

    auto [infoFile, moduleConfigFile] = parseFomodFiles(tree);

    if (infoFile == nullptr || moduleConfigFile == nullptr) {
        return RESULT_FAILED;
    }

    // create ui & pass xml classes to ui
    auto [json, matchMod]
        = getExistingFomodJson(modName, nexusID, static_cast<int>(moduleConfigFile->installSteps.installSteps.size()));
    if (matchMod != nullptr) {
        modName.update(matchMod->name(), GUESS_USER);
    }
    auto fomodViewModel = FomodViewModel::create(mOrganizer, std::move(moduleConfigFile), std::move(infoFile));
    const auto window   = std::make_shared<FomodInstallerWindow>(this, modName, tree, mFomodPath, fomodViewModel, json);

    const QDialog::DialogCode result = showInstallerWindow(window);
    if (result == QDialog::Accepted) {
        // modname was updated in window
        mInstallerUsed                               = true;
        const std::shared_ptr<IFileTree> installTree = window->getFileInstaller()->install();
        tree                                         = installTree;
        mFomodJson = std::make_shared<nlohmann::json>(window->getFileInstaller()->generateFomodJson());
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
std::pair<std::unique_ptr<FomodInfoFile>, std::unique_ptr<ModuleConfiguration>> FomodPlusInstaller::parseFomodFiles(
    const std::shared_ptr<IFileTree>& tree)
{
    const auto fomodDir = findFomodDirectory(tree);
    if (fomodDir == nullptr) {
        logMessage(ERR, "FomodPlusInstaller::install - fomod directory not found");
        return { nullptr, nullptr };
    }

    // This is a strange place to set this value but okay for now.
    mFomodPath = fomodDir->parent()->path();

    const auto infoXML      = fomodDir->find(StringConstants::FomodFiles::INFO_XML.data(), FileTreeEntry::FILE);
    const auto moduleConfig = fomodDir->find(StringConstants::FomodFiles::MODULE_CONFIG.data(), FileTreeEntry::FILE);

    // Extract files first.
    vector<std::shared_ptr<const FileTreeEntry>> toExtract = {};
    if (moduleConfig) {
        toExtract.push_back(moduleConfig);
    } else {
        logMessage(ERR, "FomodPlusInstaller::install - error parsing moduleConfig.xml: Not Present");
        return { nullptr, nullptr };
    }
    if (infoXML) {
        toExtract.push_back(infoXML);
    }
    appendImageFiles(toExtract, tree);
    const auto paths = manager()->extractFiles(toExtract);

    auto moduleConfiguration = std::make_unique<ModuleConfiguration>();
    try {
        moduleConfiguration->deserialize(paths.at(0));
    } catch (XmlParseException& e) {
        logMessage(ERR, std::format("FomodPlusInstaller::install - error parsing moduleConfig.xml: {}", e.what()));
        return { nullptr, nullptr };
    }

    auto infoFile = std::make_unique<FomodInfoFile>();
    if (infoXML) {
        try {
            infoFile->deserialize(paths.at(1));
        } catch (XmlParseException& e) {
            logMessage(ERR, std::format("FomodPlusInstaller::install - error parsing info.xml: {}", e.what()));
        }
    }

    return { std::move(infoFile), std::move(moduleConfiguration) };
}

// Taken from https://github.com/ModOrganizer2/modorganizer-installer_fomod/blob/master/src/installerfomod.cpp#L123
void FomodPlusInstaller::appendImageFiles(
    vector<shared_ptr<const FileTreeEntry>>& entries, const shared_ptr<const IFileTree>& tree)
{
    static std::set<QString, FileNameComparator> imageSuffixes { "png", "jpg", "jpeg", "gif", "bmp" };
    for (auto entry : *tree) {
        if (entry->isDir()) {
            appendImageFiles(entries, entry->astree());
        } else if (imageSuffixes.contains(entry->suffix())) {
            entries.push_back(entry);
        }
    }
}

void FomodPlusInstaller::onInstallationStart(
    QString const& archive, const bool reinstallation, IModInterface* currentMod)
{
    IPluginInstallerSimple::onInstallationStart(archive, reinstallation, currentMod);
}

void FomodPlusInstaller::onInstallationEnd(const EInstallResult result, IModInterface* newMod)
{
    IPluginInstallerSimple::onInstallationEnd(result, newMod);

    // Update the meta.ini file with the fomod information
    if (mFomodJson != nullptr && result == RESULT_SUCCESS && newMod != nullptr && mInstallerUsed) {
        newMod->setPluginSetting(this->name(), "fomod", mFomodJson->dump().c_str());
        mOrganizer->refresh();
    }
    clearPriorInstallData();
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
    const QString errorText = "Mod Name: \n"
                              "Nexus ID: "
                              "\n"
                              "Exception: "
        + QString(e.what()) + "\n";

    QMessageBox msgBox;
    msgBox.setWindowTitle("FOMOD Plus Error :(");
    msgBox.setTextFormat(Qt::RichText);
    msgBox.setText("Sorry this happened. Please copy the following error and report it to me on Nexus or GitHub.\n"
                   "<pre style='background-color: #f0f0f0; padding: 10px;'>"
        + errorText + "</pre>");
    msgBox.setIcon(QMessageBox::Critical);
    msgBox.exec();
}
