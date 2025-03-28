#pragma once

#include "../../installer/lib/Logger.h"

#include <FomodDb.h>
#include <imodinterface.h>
#include <imoinfo.h>
#include <ifiletree.h>

struct AvailablePatch {
    FomodOption fomod_option;
    std::string installer_name;
    std::string patch_for_mod;
    bool installed = false;
    bool hidden    = false;
};

class PatchFinder {
public:
    explicit PatchFinder(MOBase::IOrganizer* m_organizer) : m_organizer(m_organizer)
    {
        mFomodDb = std::make_unique<FomodDB>(m_organizer->basePath().toStdString());
        logMessage(DEBUG, "mFomodDb loaded.");
        m_organizer->onUserInterfaceInitialized([this](QMainWindow*){
            populateInstalledPlugins();
            logMessage(DEBUG, "patches populated.");
        });
    }

    std::vector<AvailablePatch> getAvailablePatchesForMod(const MOBase::IModInterface* mod);

private:
    Logger& log = Logger::getInstance();
    MOBase::IOrganizer* m_organizer;
    std::unique_ptr<FomodDB> mFomodDb;
    // Map of { pluginPtr: [1.esp, 2.esp, 3.esp] }
    std::unordered_map<const MOBase::IModInterface*, std::vector<std::string> > m_installedPlugins;

    void populateInstalledPlugins();

    void logMessage(const LogLevel level, const std::string& message) const
    {
        log.logMessage(level, "[PATCHFINDER] " + message);
    }
};