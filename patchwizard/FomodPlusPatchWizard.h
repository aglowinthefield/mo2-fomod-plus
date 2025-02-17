#ifndef FOMODPLUSPATCHWIZARD_H
#define FOMODPLUSPATCHWIZARD_H

#include "ModListItemModel.h"
#include "util.h"
#include "../installer/lib/Logger.h"

#include <iplugin.h>
#include <iplugintool.h>
#include <qtreeview.h>

class FomodPlusPatchWizard final : public MOBase::IPluginTool {
    Q_OBJECT
    Q_INTERFACES(MOBase::IPlugin MOBase::IPluginTool)
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    Q_PLUGIN_METADATA(IID "io.clearing.FomodPlusPatchWizard" FILE "fomodpluspatchwizard.json")
#endif

public:
    bool init(MOBase::IOrganizer* organizer) override;

    [[nodiscard]] QString name() const override;

    [[nodiscard]] QString author() const override;

    [[nodiscard]] QString description() const override;

    [[nodiscard]] MOBase::VersionInfo version() const override;

    [[nodiscard]] QList<MOBase::PluginSetting> settings() const override;

    [[nodiscard]] QString displayName() const override;

    [[nodiscard]] QString tooltip() const override;

    [[nodiscard]] QIcon icon() const override;

    void display() const override;

private slots:
    void onSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected) const;

private:
    Logger& log = Logger::getInstance();
    MOBase::IOrganizer* mOrganizer{};
    QDialog* mDialog{};

    void logMessage(const LogLevel level, const std::string& message) const
    {
        log.logMessage(level, "[PATCH WIZARD] " + message);
    }

    void createDialog();

    QTreeView* createTreeView(QWidget* parent) const;

    ModListItemModel* createModel() const;

    [[nodiscard]] std::shared_ptr<ModListItem> createModListItemForMod(const QString& mod) const;

    std::vector<FomodNotes> getAllModsWithFomodNotes() const;

    /**
     * Creates a map keyed by mod name, where the value is an array of mods in the LO that seemingly have patches for this mod.
     *
     * @param pluginNames
     * @return The map
     */
    [[nodiscard]] PluginToMentionsMap populatePatches(const std::vector<QString>& pluginNames) const;

    static FomodNotes parseFomodNotes(const QString& modName, const QString& notes);
};


#endif //FOMODPLUSPATCHWIZARD_H