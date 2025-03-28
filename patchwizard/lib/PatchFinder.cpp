#include "PatchFinder.h"

std::vector<AvailablePatch> PatchFinder::getAvailablePatchesForMod(const MOBase::IModInterface* mod)
{
    std::vector<AvailablePatch> available_patches = {};
    // Get all installed patches for the mod from the cache
    if (const auto it = m_installedPlugins.find(mod); it == m_installedPlugins.end()) {
        return available_patches;
    }

    return available_patches;
}

void PatchFinder::populateInstalledPlugins()
{
    for (const auto& modName : m_organizer->modList()->allMods()) {
        const auto mod = m_organizer->modList()->getMod(modName);
        const auto mod_tree = mod->fileTree();
        for (auto it = mod_tree->begin() ; it != mod_tree->end(); ++it) {
            if ((*it)->isFile() && isPluginFile((*it)->name())) {
                std::cout << "Plugin: " << (*it)->name().toStdString() << std::endl;
                m_installedPlugins[mod].emplace_back((*it)->name().toStdString());
            }
        }
    }
}