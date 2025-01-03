﻿#ifndef INSTALLERFOMODPLUS_H
#define INSTALLERFOMODPLUS_H

#include "stringconstants.h"

#include <iplugin.h>
#include <iplugininstaller.h>
#include <iplugininstallersimple.h>
#include <QDialog>

#include "FomodInstallerWindow.h"

class FomodInstallerWindow;

using namespace MOBase;

class InstallerFomodPlus final : public IPluginInstallerSimple
{
  Q_OBJECT
  Q_INTERFACES(MOBase::IPlugin MOBase::IPluginInstaller MOBase::IPluginInstallerSimple)

  #if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    Q_PLUGIN_METADATA(IID "io.clearing.FomodPlus" FILE "fomodplus.json")
  #endif

public:
  bool init(IOrganizer *organizer) override;

  // constant values
  [[nodiscard]] QString name() const override { return StringConstants::Plugin::NAME; }
  [[nodiscard]] QString author() const override { return StringConstants::Plugin::AUTHOR; }
  [[nodiscard]] QString description() const override { return StringConstants::Plugin::DESCRIPTION; }
  [[nodiscard]] VersionInfo version() const override { return {1, 0, 0, VersionInfo::RELEASE_FINAL}; }
  [[nodiscard]] unsigned int priority() const override { return 120; /* Above installer_fomod's highest priority. */ }
  [[nodiscard]] bool isManualInstaller() const override { return false; }

  [[nodiscard]] bool isArchiveSupported(std::shared_ptr<const IFileTree> tree) const override;
  [[nodiscard]] QList<PluginSetting> settings() const override;
  EInstallResult install(GuessedValue<QString> &modName, std::shared_ptr<IFileTree> &tree, QString &version, int &nexusID) override;
  void onInstallationStart(QString const& archive, bool reinstallation, IModInterface* currentMod) override;
  void onInstallationEnd(EInstallResult result, IModInterface* newMod) override;

private:

  IOrganizer* m_Organizer = nullptr;

  /**
 * @brief Retrieve the tree entry corresponding to the fomod directory.
 *
 * @param tree Tree to look-up the directory in.
 *
 * @return the entry corresponding to the fomod directory in the tree, or a null
 * pointer if the entry was not found.
 */
  [[nodiscard]] static std::shared_ptr<const IFileTree> findFomodDirectory(const std::shared_ptr<const IFileTree> &tree);


  [[nodiscard]] static QDialog::DialogCode showInstallerWindow(const std::shared_ptr<FomodInstallerWindow>& window);
};

#endif //INSTALLERFOMODPLUS_H
