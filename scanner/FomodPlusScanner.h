#ifndef FOMODPLUSSCANNER_H
#define FOMODPLUSSCANNER_H

#include <iplugin.h>
#include <iplugintool.h>

#include <QDialog>
#include <QProgressBar>

using namespace MOBase;

class FomodPlusScanner final : public IPluginTool {
  Q_OBJECT
  Q_INTERFACES(MOBase::IPlugin MOBase::IPluginTool)
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
  Q_PLUGIN_METADATA(IID "io.clearing.FomodPlusScanner" FILE "fomodplusscanner.json")
#endif

public:
  ~FomodPlusScanner() override {
    delete mDialog;
    mDialog      = nullptr;
    mProgressBar = nullptr;
  }

  bool init(IOrganizer* organizer) override;

  void onScanClicked() const;

  void onDeleteClicked() const;

  void cleanup() const;

  [[nodiscard]] QString     name() const override { return "FOMOD Scanner"; }
  [[nodiscard]] QString     author() const override { return "clearing"; }
  [[nodiscard]] QString     description() const override { return "Scans modlist for files installed via FOMOD"; }
  [[nodiscard]] VersionInfo version() const override { return VersionInfo(1, 0, 0, VersionInfo::RELEASE_FINAL); }

  [[nodiscard]] QList<PluginSetting> settings() const override { return {}; }

  [[nodiscard]] QString displayName() const override { return "FOMOD Scanner"; }

  [[nodiscard]] QString tooltip() const override { return "Scan modlist for files installed via FOMOD"; }

  [[nodiscard]] QIcon icon() const override { return QIcon(":/fomod/hat"); }

  void display() const override;

  int scanLoadOrder(const std::function<bool(IModInterface*)>& callback) const;

  int openInstallationArchive(const IModInterface* mod) const;

  static bool setFomodInfoForMod(IModInterface* mod);

  static bool removeFomodInfoFromMod(IModInterface* mod);

private:
  QDialog*      mDialog{nullptr};
  QProgressBar* mProgressBar{nullptr};
  IOrganizer*   mOrganizer{nullptr};
};

#endif  // FOMODPLUSSCANNER_H