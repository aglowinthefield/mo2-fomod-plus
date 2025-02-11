#ifndef FOMODPLUSPATCHWIZARD_H
#define FOMODPLUSPATCHWIZARD_H


#include "ModListItemModel.h"
#include "../installer/lib/Logger.h"

#include <iplugin.h>
#include <iplugintool.h>
#include <qtreeview.h>

using PluginToMentionsMap = std::unordered_map<QString, std::vector<MOBase::IModInterface*> >;

class FomodPlusPatchWizard final : public MOBase::IPluginTool {
    Q_OBJECT
    Q_INTERFACES(MOBase::IPlugin MOBase::IPluginTool)
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    Q_PLUGIN_METADATA(IID "io.clearing.FomodPlusPatchWizard" FILE "fomodpluspatchwizard.json")
#endif
public:
    bool init(MOBase::IOrganizer* organizer) override;

    QString name() const override;

    QString author() const override;

    QString description() const override;

    MOBase::VersionInfo version() const override;

    QList<MOBase::PluginSetting> settings() const override;

    QString displayName() const override;

    QString tooltip() const override;

    QIcon icon() const override;

    void display() const override;

    void createDialog();

    QTreeView* createTreeView(QWidget* parent);

    ModListItemModel* createModel();

    std::shared_ptr<ModListItem> createModListItemForMod(const QString &mod);

    PluginToMentionsMap populatePatches(std::shared_ptr<ModListItem> item);

private:
    Logger& log = Logger::getInstance();
    MOBase::IOrganizer* mOrganizer{};
    QDialog* mDialog{};

    void logMessage(LogLevel level, const std::string& message) {
        log.logMessage(level, "[PATCH WIZARD] " + message);
    }

};



#endif //FOMODPLUSPATCHWIZARD_H
