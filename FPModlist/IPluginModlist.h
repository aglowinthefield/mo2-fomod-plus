#pragma once

#include <iplugin.h>

class IPluginModlist final : public QObject, public MOBase::IPlugin
{
    Q_INTERFACES(MOBase::IPlugin)

public:
    bool init(MOBase::IOrganizer* organizer) override;

    [[nodiscard]] QString name() const override;

    [[nodiscard]] QString author() const override;

    [[nodiscard]] QString description() const override;

    [[nodiscard]] MOBase::VersionInfo version() const override;

    [[nodiscard]] QList<MOBase::PluginSetting> settings() const override;
};

Q_DECLARE_INTERFACE(IPluginModlist, "com.tannin.ModOrganizer.Plugin/2.0")