#pragma once

#include "stringutil.h"

#include <iplugin.h>
#include <iplugininstaller.h>
#include <iplugininstallersimple.h>

#include "FomodInstallerWindow.h"
#include "lib/Logger.h"
#include "xml/FomodInfoFile.h"
#include "xml/ModuleConfiguration.h"
#include <nlohmann/json.hpp>

#include <FOMODData/FomodDb.h>
#include <QDialog>
#include <integration/FomodDataContent.h>

class FomodInstallerWindow;

using namespace MOBase;
using namespace std;

using ParsedFilesTuple = std::tuple<std::unique_ptr<FomodInfoFile>, std::unique_ptr<ModuleConfiguration>, QStringList>;

class FomodPlusInstaller final : public IPluginInstallerSimple {
    Q_OBJECT
    Q_INTERFACES(MOBase::IPlugin MOBase::IPluginInstaller)
    Q_PLUGIN_METADATA(IID "io.clearing.FomodPlus" FILE "fomodplus.json")

  public:
    bool init(IOrganizer* organizer) override;

    // constant values
    [[nodiscard]] QString name() const override { return StringConstants::Plugin::NAME.data(); }
    [[nodiscard]] QString author() const override { return StringConstants::Plugin::AUTHOR.data(); }
    [[nodiscard]] QString description() const override { return StringConstants::Plugin::DESCRIPTION.data(); }
    [[nodiscard]] VersionInfo version() const override { return { 1, 0, 0, VersionInfo::RELEASE_FINAL }; }

    [[nodiscard]] unsigned int priority() const override { return 999; /* Above installer_fomod's highest priority. */ }

    [[nodiscard]] std::vector<std::shared_ptr<const IPluginRequirement>> requirements() const override;

    [[nodiscard]] bool isManualInstaller() const override { return false; }

    [[nodiscard]] bool isArchiveSupported(std::shared_ptr<const IFileTree> tree) const override;

    [[nodiscard]] QList<PluginSetting> settings() const override;

    std::pair<nlohmann::json, IModInterface*> getExistingFomodJson(
        const GuessedValue<QString>& modName, const int& nexusId, const int& stepsInCurrentFomod) const;

    void clearPriorInstallData();

    EInstallResult install(
        GuessedValue<QString>& modName, std::shared_ptr<IFileTree>& tree, QString& version, int& nexusID) override;

    void onInstallationStart(QString const& archive, bool reinstallation, IModInterface* currentMod) override;

    void onInstallationEnd(EInstallResult result, IModInterface* newMod) override;

    [[nodiscard]] bool shouldShowImages() const;

    [[nodiscard]] bool shouldShowNotifications() const;
    bool shouldShowSidebarFilter() const;

    [[nodiscard]] bool shouldAutoRestoreChoices() const;

    [[nodiscard]] bool isWizardIntegrated() const;

    void toggleShouldShowImages() const;

    QString getSelectedColor() const;

    [[nodiscard]] QString getNexusGameName() const;

  private:
    Logger& log            = Logger::getInstance();
    IOrganizer* mOrganizer = nullptr;
    QString mFomodPath {};
    std::shared_ptr<nlohmann::json> mFomodJson { nullptr };
    bool mInstallerUsed { false };
    std::shared_ptr<FomodDataContent> mFomodContent { nullptr };
    std::unique_ptr<FomodDB> mFomodDb;

    /**
     * @brief Retrieve the tree entry corresponding to the fomod directory.
     *
     * @param tree Tree to look-up the directory in.
     *
     * @return the entry corresponding to the fomod directory in the tree, or a null
     * pointer if the entry was not found.
     */
    [[nodiscard]] static shared_ptr<const IFileTree> findFomodDirectory(const shared_ptr<const IFileTree>& tree);

    [[nodiscard]] static QDialog::DialogCode showInstallerWindow(const shared_ptr<FomodInstallerWindow>& window);

    [[nodiscard]] ParsedFilesTuple parseFomodFiles(const shared_ptr<IFileTree>& tree);

    static void appendImageFiles(
        vector<shared_ptr<const FileTreeEntry>>& entries, const shared_ptr<const IFileTree>& tree);

    void appendPluginFiles(vector<shared_ptr<const FileTreeEntry>>& entries, const shared_ptr<const IFileTree>& tree);

    void setupUiInjection() const;
    void toggleFeature(bool enabled) const;

    [[nodiscard]] bool shouldFallbackToLegacyInstaller() const;

    void logMessage(const LogLevel level, const std::string& message) const
    {
        log.logMessage(level, "[INSTALLER] " + message);
    }
};
