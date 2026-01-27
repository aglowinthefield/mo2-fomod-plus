#pragma once

#include "ArchiveExtractor.h"
#include "FomodDB.h"
#include "stringutil.h"
#include "xml/ModuleConfiguration.h"

#include <QDir>
#include <QString>
#include <functional>
#include <imodinterface.h>
#include <imoinfo.h>
#include <nlohmann/json.hpp>

struct RescanResult {
    int totalModsProcessed = 0;
    int successfullyScanned = 0;
    int missingArchives = 0;
    int parseErrors = 0;
    std::vector<std::string> failedMods;
};

/**
 * Orchestrates rescanning of all mods with stored FOMOD choices to repopulate the database.
 * Used when fomod.db is missing or needs to be regenerated from existing installations.
 */
class FomodRescan {
public:
    using ProgressCallback = std::function<void(int current, int total, const QString& modName)>;

    FomodRescan(MOBase::IOrganizer* organizer, FomodDB* db)
        : mOrganizer(organizer), mFomodDb(db) {}

    /**
     * Scan all mods that have stored FOMOD Plus choices and repopulate the database.
     * @param progressCallback Optional callback for progress updates
     * @return RescanResult with statistics about the scan
     */
    RescanResult scanAllModsWithChoices(const ProgressCallback& progressCallback = nullptr)
    {
        RescanResult result;

        const auto modList = mOrganizer->modList();
        if (!modList) {
            return result;
        }

        // First pass: gather all mods with stored choices
        std::vector<MOBase::IModInterface*> modsWithChoices;
        for (const auto& modName : modList->allMods()) {
            auto* mod = modList->getMod(modName);
            if (mod && hasStoredChoices(mod)) {
                modsWithChoices.push_back(mod);
            }
        }

        result.totalModsProcessed = static_cast<int>(modsWithChoices.size());

        // Second pass: process each mod
        int current = 0;
        for (auto* mod : modsWithChoices) {
            current++;
            if (progressCallback) {
                progressCallback(current, result.totalModsProcessed, mod->name());
            }

            const auto scanResult = processMod(mod);
            switch (scanResult) {
                case ScanOutcome::Success:
                    result.successfullyScanned++;
                    break;
                case ScanOutcome::MissingArchive:
                    result.missingArchives++;
                    result.failedMods.push_back(mod->name().toStdString() + " (missing archive)");
                    break;
                case ScanOutcome::ParseError:
                    result.parseErrors++;
                    result.failedMods.push_back(mod->name().toStdString() + " (parse error)");
                    break;
                case ScanOutcome::NoFomod:
                    result.failedMods.push_back(mod->name().toStdString() + " (no FOMOD)");
                    break;
            }
        }

        // Save the database
        mFomodDb->saveToFile();

        return result;
    }

private:
    MOBase::IOrganizer* mOrganizer;
    FomodDB* mFomodDb;

    enum class ScanOutcome {
        Success,
        MissingArchive,
        ParseError,
        NoFomod
    };

    /**
     * Check if a mod has stored FOMOD Plus choices (non-zero pluginSetting).
     */
    bool hasStoredChoices(MOBase::IModInterface* mod) const
    {
        const auto fomodData = mod->pluginSetting(
            StringConstants::Plugin::NAME.data(), "fomod", 0);

        if (!fomodData.isValid() || fomodData.isNull()) {
            return false;
        }

        // Check if it's actually valid JSON with steps
        try {
            const auto json = nlohmann::json::parse(fomodData.toString().toStdString());
            return json.contains("steps") && json["steps"].is_array() && !json["steps"].empty();
        } catch (...) {
            return false;
        }
    }

    /**
     * Get the stored choices JSON from a mod's pluginSetting.
     */
    nlohmann::json getStoredChoices(MOBase::IModInterface* mod) const
    {
        const auto fomodData = mod->pluginSetting(
            StringConstants::Plugin::NAME.data(), "fomod", 0);

        try {
            return nlohmann::json::parse(fomodData.toString().toStdString());
        } catch (...) {
            return nlohmann::json();
        }
    }

    /**
     * Process a single mod: extract archive, parse FOMOD, create DB entry with selection states.
     */
    ScanOutcome processMod(MOBase::IModInterface* mod)
    {
        // Get the archive path
        const auto installationFile = mod->installationFile();
        if (installationFile.isEmpty()) {
            return ScanOutcome::MissingArchive;
        }

        const auto downloadsPath = mOrganizer->downloadsPath();
        const auto archivePath = QDir(installationFile).isAbsolute()
            ? installationFile
            : downloadsPath + "/" + installationFile;

        if (!QFile::exists(archivePath)) {
            return ScanOutcome::MissingArchive;
        }

        // Extract FOMOD data from archive
        auto extractionResult = ArchiveExtractor::extractFomodData(archivePath);
        if (!extractionResult.success) {
            return ScanOutcome::ParseError;
        }

        // Parse ModuleConfiguration
        auto moduleConfig = std::make_unique<ModuleConfiguration>();
        try {
            if (!moduleConfig->deserialize(extractionResult.moduleConfigPath)) {
                return ScanOutcome::ParseError;
            }
        } catch (...) {
            return ScanOutcome::ParseError;
        }

        // Get the mod's Nexus ID
        const int modId = mod->nexusId();

        // Create FomodDbEntry using existing logic
        auto entry = FomodDB::getEntryFromFomod(
            moduleConfig.get(),
            extractionResult.pluginPaths,
            modId
        );

        if (!entry || entry->getOptions().empty()) {
            return ScanOutcome::NoFomod;
        }

        // Apply selection states from stored choices
        const auto choices = getStoredChoices(mod);
        applySelectionsToEntry(*entry, choices);

        // Add to database (upsert)
        mFomodDb->addEntry(entry, true);

        return ScanOutcome::Success;
    }

    /**
     * Apply user selection states to a FomodDbEntry based on stored choices JSON.
     *
     * Choices JSON format:
     * {
     *   "steps": [{
     *     "name": "Step Name",
     *     "groups": [{
     *       "name": "Group Name",
     *       "plugins": ["Selected Plugin 1"],
     *       "deselected": ["Manually Deselected Plugin"]
     *     }]
     *   }]
     * }
     */
    void applySelectionsToEntry(FomodDbEntry& entry, const nlohmann::json& choices)
    {
        if (!choices.contains("steps") || !choices["steps"].is_array()) {
            // No choices data - mark all as Available
            for (auto& option : entry.getOptionsMutable()) {
                option.selectionState = SelectionState::Available;
            }
            return;
        }

        // Build a lookup map for quick matching: stepName/groupName/pluginName -> state
        struct PluginState {
            bool selected = false;
            bool deselected = false;
        };
        std::map<std::string, PluginState> stateMap;

        for (const auto& step : choices["steps"]) {
            if (!step.contains("name") || !step.contains("groups")) continue;
            const std::string stepName = step["name"];

            for (const auto& group : step["groups"]) {
                if (!group.contains("name")) continue;
                const std::string groupName = group["name"];

                // Process selected plugins
                if (group.contains("plugins") && group["plugins"].is_array()) {
                    for (const auto& plugin : group["plugins"]) {
                        const std::string pluginName = plugin;
                        const auto key = stepName + "/" + groupName + "/" + pluginName;
                        stateMap[key].selected = true;
                    }
                }

                // Process deselected plugins
                if (group.contains("deselected") && group["deselected"].is_array()) {
                    for (const auto& plugin : group["deselected"]) {
                        const std::string pluginName = plugin;
                        const auto key = stepName + "/" + groupName + "/" + pluginName;
                        stateMap[key].deselected = true;
                    }
                }
            }
        }

        // Apply states to options
        for (auto& option : entry.getOptionsMutable()) {
            const auto key = option.step + "/" + option.group + "/" + option.name;

            if (auto it = stateMap.find(key); it != stateMap.end()) {
                if (it->second.selected) {
                    option.selectionState = SelectionState::Selected;
                } else if (it->second.deselected) {
                    option.selectionState = SelectionState::Deselected;
                } else {
                    option.selectionState = SelectionState::Available;
                }
            } else {
                // Plugin not found in choices - mark as Available
                option.selectionState = SelectionState::Available;
            }
        }
    }
};
