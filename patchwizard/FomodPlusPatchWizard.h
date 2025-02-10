#ifndef FOMODPLUSPATCHWIZARD_H
#define FOMODPLUSPATCHWIZARD_H


#include <iplugin.h>
#include <iplugintool.h>

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

private:
    MOBase::IOrganizer* mOrganizer{};
    QDialog* mDialog{};

};



#endif //FOMODPLUSPATCHWIZARD_H
