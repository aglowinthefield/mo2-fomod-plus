#pragma once
#include "../installer/lib/Logger.h"
#include "lib/PatchFinder.h"

#include <iplugintool.h>
#include <qtmetamacros.h>

using namespace MOBase;

class FomodPlusPatchWizard final : public IPluginTool {
    Q_OBJECT
    Q_INTERFACES(MOBase::IPlugin MOBase::IPluginTool)
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    Q_PLUGIN_METADATA(IID "io.clearing.FomodPlusPatchWizard" FILE "fomodpluspatchwizard.json")
#endif

public:
    bool init(IOrganizer* organizer) override;

    [[nodiscard]] QString name() const override { return tr("Patch Wizard"); };

    [[nodiscard]] QString author() const override { return "clearing"; };

    [[nodiscard]] QString description() const override { return tr("Find missing patches from FOMODs in your load order."); };

    [[nodiscard]] VersionInfo version() const override { return { 1, 0, 0, VersionInfo::RELEASE_BETA }; };

    [[nodiscard]] QList<PluginSetting> settings() const override { return {}; };

    [[nodiscard]] QString displayName() const override { return tr("Patch Wizard"); };

    [[nodiscard]] QString tooltip() const override { return tr("Find missing patches from FOMODs in your load order."); };

    [[nodiscard]] QIcon icon() const override { return QIcon(":/fomod/hat"); }

    void display() const override;

private:
    Logger& log = Logger::getInstance();
    QDialog* mDialog{ nullptr };
    IOrganizer* mOrganizer{ nullptr };
    std::unique_ptr<PatchFinder> mPatchFinder{ nullptr };
    std::vector<AvailablePatch> mAvailablePatches;

    void setupEmptyState() const;
    void setupPatchList() const;
    void onRescanClicked();

    void logMessage(const LogLevel level, const std::string& message) const
    {
        log.logMessage(level, "[PATCHFINDER] " + message);
    }

};