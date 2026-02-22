#include "PatchFinder.h"

std::vector<AvailablePatch> PatchFinder::getAvailablePatchesForMod(const MOBase::IModInterface* mod)
{
    std::vector<AvailablePatch> available_patches = {};

    // Verify that the mod has plugins in some shape or form.
    if (const auto it = m_installedPlugins.find(mod); it == m_installedPlugins.end()) {
        return available_patches;
    }

    const auto modName = mod->name().toStdString();

    // Look through the database
    for (const auto& fomodDbEntries = mFomodDb->getEntries(); const auto& entry : fomodDbEntries) {
        for (const auto& option : entry->getOptions()) {
            // Skip options that are already selected/installed
            if (option.selectionState == SelectionState::Selected) {
                continue;
            }

            // If we have a specific plugin file, check if it's already installed
            if (!option.fileName.empty()) {
                const auto fileName = option.fileName.substr(option.fileName.find_last_of("/\\") + 1);
                if (m_installedPluginsCacheSet.contains(fileName)) {
                    continue;
                }
            }

            // Check if this patch is related to the current mod:
            // 1. Masters-based: the patch lists this mod as a master
            bool masterRelated = std::ranges::any_of(
                option.masters, [&modName](const std::string& master) { return master == modName; });

            // 2. Condition-based: the patch has a file dependency referencing a plugin from this mod
            bool conditionRelated = false;
            if (!masterRelated && !option.typePatterns.empty()) {
                if (const auto modIt = m_installedPlugins.find(mod); modIt != m_installedPlugins.end()) {
                    for (const auto& modPlugin : modIt->second) {
                        for (const auto& pattern : option.typePatterns) {
                            if (std::ranges::any_of(pattern.dependencies.fileDependencies,
                                    [&modPlugin](const StoredFileDependency& fd) {
                                        return fd.file == modPlugin;
                                    })) {
                                conditionRelated = true;
                                break;
                            }
                        }
                        if (conditionRelated)
                            break;
                    }
                }
            }

            if (!masterRelated && !conditionRelated) {
                continue;
            }

            // Check if all masters are installed (existing logic)
            bool mastersMatch = !option.masters.empty()
                && std::ranges::all_of(option.masters,
                    [this](const std::string& master) { return m_installedPluginsCacheSet.contains(master); });

            // Check if conditions suggest this patch (new logic)
            bool conditionsMatch = false;
            if (!option.typePatterns.empty() && mPluginStateResolver) {
                auto resolvedType
                    = ConditionEvaluator::resolveMatchingType(option.typePatterns, mPluginStateResolver);
                conditionsMatch = (resolvedType == "Recommended" || resolvedType == "Required");
            }

            if (mastersMatch || conditionsMatch) {
                AvailablePatch patch {
                    option, entry->getDisplayName(), mod->name().toStdString(),
                    false, // not installed
                    false, // not hidden
                    option.selectionState == SelectionState::Deselected // userDeselected
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

bool PatchFinder::isSuggested(const FomodOption& option) const
{
    if (option.selectionState != SelectionState::Available
        && option.selectionState != SelectionState::Unknown)
        return false;

    // If we have a specific plugin fileName, check if it's already installed
    if (!option.fileName.empty()) {
        const auto fileName = option.fileName.substr(option.fileName.find_last_of("/\\") + 1);
        if (m_installedPluginsCacheSet.contains(fileName))
            return false;
    }

    // Masters signal (existing logic) — requires a known plugin file
    const bool mastersMatch = !option.fileName.empty() && !option.masters.empty()
        && std::ranges::all_of(option.masters,
            [this](const std::string& master) { return m_installedPluginsCacheSet.contains(master); });

    // Conditions signal (first-match semantics) — works for both file and folder installs
    bool conditionsMatch = false;
    if (!option.typePatterns.empty() && mPluginStateResolver) {
        const auto resolvedType
            = ConditionEvaluator::resolveMatchingType(option.typePatterns, mPluginStateResolver);
        conditionsMatch = (resolvedType == "Recommended" || resolvedType == "Required");
    }

    return mastersMatch || conditionsMatch;
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

    // Create a cached resolver for condition evaluation using the live plugin list
    mPluginStateResolver = makeCachedResolver([this](const std::string& fileName) -> std::string {
        if (!isPluginFile(fileName)) {
            const auto resolved = m_organizer->resolvePath(QString::fromStdString(fileName));
            return resolved.isEmpty() ? "Missing" : "Active";
        }
        const auto state = m_organizer->pluginList()->state(QString::fromStdString(fileName));
        if (state & MOBase::IPluginList::STATE_ACTIVE)
            return "Active";
        if (state & MOBase::IPluginList::STATE_INACTIVE)
            return "Inactive";
        return "Missing";
    });
}
