#include "PatchFinder.h"

std::vector<AvailablePatch> PatchFinder::getAvailablePatchesForMod(const MOBase::IModInterface* mod)
{
    std::vector<AvailablePatch> available_patches = {};

    // Verify that the mod has plugins in some shape or form.
    if (const auto it = m_installedPlugins.find(mod); it == m_installedPlugins.end()) {
        return available_patches;
    }

    // Look through the database
    for (const auto& fomodDbEntries = mFomodDb->getEntries(); const auto& entry : fomodDbEntries) {
        for (const auto& option : entry->getOptions()) {
            // Skip options that are already selected/installed
            if (option.selectionState == SelectionState::Selected) {
                continue;
            }

            // Extract just the filename from the path (handle both / and \)
            const auto fileName = option.fileName.substr(option.fileName.find_last_of("/\\") + 1);

            // Skip if already installed in the modlist
            if (m_installedPluginsCacheSet.contains(fileName)) {
                continue;
            }

            // Technically without this check, we're doing a lookup for the entire modlist, which actually
            // kinda works, but isn't the intention of this function. Might want to consider reworking
            // it to map to mod names with one pass of the DB instead of a DB pass per mod.
            if (!std::ranges::any_of(option.masters, [&mod](const std::string& master) {
                return master == mod->name().toStdString();
            })) {
                continue;
            }

            // If we have all of this patch's masters in our modlist, and it's not installed, add it to the results.
            if (std::ranges::all_of(option.masters, [this](const std::string& master) {
                return m_installedPluginsCacheSet.contains(master);
            })) {
                AvailablePatch patch{
                    option,
                    entry->getDisplayName(),
                    mod->name().toStdString(),
                    false,  // not installed
                    false,  // not hidden
                    option.selectionState == SelectionState::Deselected  // userDeselected
                };
                available_patches.push_back(patch);
            }
        }
    }

    return available_patches;
}

std::vector<AvailablePatch> PatchFinder::getAvailablePatchesForModList()
{
    std::vector<AvailablePatch> available_patches = {};
    for (const auto& modName : m_organizer->modList()->allMods()) {
        const auto mod = m_organizer->modList()->getMod(modName);
        if (mod == nullptr) {
            continue;
        }
        for (const auto& available_patch : getAvailablePatchesForMod(mod)) {
            available_patches.emplace_back(available_patch);
        }
    }

    return available_patches;
}

void PatchFinder::populateInstalledPlugins()
{
    for (const auto& modName : m_organizer->modList()->allMods()) {
        const auto mod      = m_organizer->modList()->getMod(modName);
        const auto mod_tree = mod->fileTree();
        for (auto it = mod_tree->begin(); it != mod_tree->end(); ++it) {
            if ((*it)->isFile() && isPluginFile((*it)->name())) {
                std::cout << "Plugin: " << (*it)->name().toStdString() << std::endl;
                m_installedPlugins[mod].emplace_back((*it)->name().toStdString());
                m_installedPluginsCacheSet.insert((*it)->name().toStdString());
            }
        }
    }
}