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
    int totalModsProcessed  = 0;
    int successfullyScanned = 0;
    int skippedNoOptions    = 0; // FOMODs with no installSteps/options
    int missingArchives     = 0;
    int parseErrors         = 0;
    std::vector<std::string> failedMods; // Only actual failures, not skipped mods
};

/**
 * Orchestrates rescanning of all mods with stored FOMOD choices to repopulate the database.
 * Used when fomod.db is missing or needs to be regenerated from existing installations.
 */
class FomodRescan {
  public:
    using ProgressCallback = std::function<void(int current, int total, const QString& modName)>;

    FomodRescan(MOBase::IOrganizer* organizer, FomodDB* db)
        : mOrganizer(organizer)
        , mFomodDb(db)
    {
    }

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

        // Masters cache - avoids re-reading the same plugin files (e.g., Lux.esp) across archives
        MastersCache mastersCache;

        // Second pass: process each mod
        int current = 0;
        for (auto* mod : modsWithChoices) {
            current++;
            if (progressCallback) {
                progressCallback(current, result.totalModsProcessed, mod->name());
            }

            const auto [outcome, errorDetail] = processMod(mod, mastersCache);
            switch (outcome) {
            case ScanOutcome::Success:
                result.successfullyScanned++;
                break;
            case ScanOutcome::MissingArchive:
                result.missingArchives++;
                result.failedMods.push_back(mod->name().toStdString() + " (missing archive)");
                break;
            case ScanOutcome::ParseError:
                result.parseErrors++;
                result.failedMods.push_back(mod->name().toStdString() + " (parse error: " + errorDetail + ")");
                break;
            case ScanOutcome::NoOptions:
                // Not a failure - FOMOD has no installSteps/options to track
                result.skippedNoOptions++;
                break;
            }
        }

        // Save the database
        mFomodDb->saveToFile();

        // Log cache effectiveness
        std::cout << "[FomodRescan] Masters cache: " << mastersCache.size() << " unique plugins cached" << std::endl;

        return result;
    }

  private:
    MOBase::IOrganizer* mOrganizer;
    FomodDB* mFomodDb;

    enum class ScanOutcome {
        Success,
        MissingArchive,
        ParseError,
        NoOptions // FOMOD exists but has no installSteps/options to track
    };

    /**
     * Check if a mod has stored FOMOD Plus choices (non-zero pluginSetting).
     */
    bool hasStoredChoices(MOBase::IModInterface* mod) const
    {
        const auto fomodData = mod->pluginSetting(StringConstants::Plugin::NAME.data(), "fomod", 0);

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
        const auto fomodData = mod->pluginSetting(StringConstants::Plugin::NAME.data(), "fomod", 0);

        try {
            return nlohmann::json::parse(fomodData.toString().toStdString());
        } catch (...) {
            return nlohmann::json();
        }
    }

    using ProcessResult = std::pair<ScanOutcome, std::string>;

    /**
     * Process a single mod: extract archive, parse FOMOD, create DB entry with selection states.
     * Returns outcome and optional error detail string.
     */
    ProcessResult processMod(MOBase::IModInterface* mod, MastersCache& cache)
    {
        // Get the archive path
        const auto installationFile = mod->installationFile();
        if (installationFile.isEmpty()) {
            return { ScanOutcome::MissingArchive, "" };
        }

        const auto downloadsPath = mOrganizer->downloadsPath();
        const auto archivePath
            = QDir(installationFile).isAbsolute() ? installationFile : downloadsPath + "/" + installationFile;

        if (!QFile::exists(archivePath)) {
            return { ScanOutcome::MissingArchive, "" };
        }

        // Extract FOMOD data from archive
        auto extractionResult = ArchiveExtractor::extractFomodData(archivePath);
        if (!extractionResult.success) {
            return { ScanOutcome::ParseError, "extraction: " + extractionResult.errorMessage.toStdString() };
        }

        // Parse ModuleConfiguration
        auto moduleConfig = std::make_unique<ModuleConfiguration>();
        try {
            if (!moduleConfig->deserialize(extractionResult.moduleConfigPath)) {
                return { ScanOutcome::ParseError, "XML deserialization failed" };
            }
        } catch (const std::exception& e) {
            return { ScanOutcome::ParseError, std::string("XML exception: ") + e.what() };
        } catch (...) {
            return { ScanOutcome::ParseError, "unknown XML exception" };
        }

        // Get the mod's Nexus ID
        const int modId = mod->nexusId();

        // Create FomodDbEntry using existing logic (with masters cache for performance)
        auto entry = FomodDB::getEntryFromFomod(moduleConfig.get(), extractionResult.pluginPaths, modId, &cache);

        if (!entry || entry->getOptions().empty()) {
            // Not a failure - FOMOD exists but has no installSteps/options to track
            return { ScanOutcome::NoOptions, "" };
        }

        // Apply selection states from stored choices
        const auto choices = getStoredChoices(mod);
        entry->applySelections(choices);

        // Add to database (upsert)
        mFomodDb->addEntry(entry, true);

        return { ScanOutcome::Success, "" };
    }

};
