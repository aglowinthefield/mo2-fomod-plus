#pragma once

#include <fstream>
#include <stringutil.h>
#include <unordered_map>

#include <QFileInfo>

#include "FomodDBEntry.h"

#include <xml/ModuleConfiguration.h>

#include "PluginReader.h"

using FOMODDBEntries = std::vector<std::shared_ptr<FomodDbEntry>>;
using MastersCache   = std::unordered_map<std::string, std::vector<std::string>>; // filename -> masters

constexpr const char* FOMOD_DB_FILE = "fomod.db";

class FomodDB {
  public:
    /**
     *
     * @param moBasePath The organizer instance's basePath() value
     * @param dbName The filename of the db. Only settable for testing purposes.
     */
    explicit FomodDB(const std::string& moBasePath, const std::string& dbName = FOMOD_DB_FILE)
    {
        dbFilePath = (std::filesystem::path(moBasePath) / dbName).string();
        loadFromFile();
    }

    // TODO: Also pull from non install steps (requiredInstallFiles or whatever, and optional);
    static std::shared_ptr<FomodDbEntry> getEntryFromFomod(
        ModuleConfiguration* fomod, std::vector<QString> pluginPaths, int modId, MastersCache* cache = nullptr)
    {
        std::vector<FomodOption> options;
        for (const auto& installStep : fomod->installSteps.installSteps) {
            for (const auto& group : installStep.optionalFileGroups.groups) {
                for (const auto& plugin : group.plugins.plugins) {
                    // Create a DB entry for every FOMOD option (not just those with plugins)
                    // This allows searching/browsing all options, with master-matching for patches

                    std::string pluginFileName;
                    std::vector<std::string> masters;

                    // Look for plugin files (.esp/.esm/.esl) to extract masters for patch matching
                    for (const auto& file : plugin.files.files) {
                        if (file.isFolder || !isPluginFile(file.source)) {
                            continue;
                        }

                        // Normalize the source path for comparison (handle both / and \)
                        QString normalizedSource = QString::fromStdString(file.source).replace('\\', '/');

                        // Find the extracted path that ends with this file
                        auto it = std::ranges::find_if(pluginPaths, [&normalizedSource](const QString& path) {
                            QString normalizedPath = path;
                            normalizedPath.replace('\\', '/');
                            return normalizedPath.endsWith(normalizedSource);
                        });
                        if (it == pluginPaths.end()) {
                            continue;
                        }

                        // Found a plugin file - read its masters (using cache if available)
                        pluginFileName = file.source;
                        const auto justFileName
                            = QFileInfo(QString::fromStdString(file.source)).fileName().toStdString();

                        if (cache) {
                            auto cacheIt = cache->find(justFileName);
                            if (cacheIt != cache->end()) {
                                // Cache hit - use cached masters
                                masters = cacheIt->second;
                            } else {
                                // Cache miss - read and cache
                                masters                = PluginReader::readMasters(it->toStdString(), true);
                                (*cache)[justFileName] = masters;
                            }
                        } else {
                            masters = PluginReader::readMasters(it->toStdString(), true);
                        }
                        break; // Use first plugin file found
                    }

                    // Extract TypeDescriptor patterns for condition-based suggestion
                    auto typePatterns = extractTypePatterns(plugin);

                    // Always create an option entry, even if no plugin files
                    options.emplace_back(plugin.name,
                        pluginFileName, // May be empty if no plugin files
                        masters, // May be empty if no plugin files
                        installStep.name, group.name, SelectionState::Unknown, std::move(typePatterns));
                }
            }
        }
        return std::make_shared<FomodDbEntry>(modId, fomod->moduleName, options);
    }

    void addEntry(const std::shared_ptr<FomodDbEntry>& entry, const bool upsert = true)
    {
        // TODO: Test this upsert.
        if (upsert) {
            const auto it = std::ranges::find_if(entries,
                [&entry](const std::shared_ptr<FomodDbEntry>& e) { return e->getModId() == entry->getModId(); });
            if (it != entries.end()) {
                *it = entry;
            } else {
                entries.emplace_back(entry);
            }
        } else {
            entries.emplace_back(entry);
        }
    }

    [[nodiscard]] const FOMODDBEntries& getEntries() { return entries; }

    void reload() { loadFromFile(); }

    void saveToFile() const
    {
        try {
            std::ofstream file(dbFilePath);
            if (!file.is_open()) {
                return;
            }

            file << toJson().dump(2); // Pretty-print with 2-space indentation
            file.close();
        } catch ([[maybe_unused]] const std::exception& e) {
            // Handle saving errors
        }
    }

    [[nodiscard]] nlohmann::json toJson() const
    {
        nlohmann::json jsonArray = nlohmann::json::array();

        for (const auto& entry : entries) {
            jsonArray.push_back(entry->toJson());
        }

        return jsonArray;
    }

  private:
    FOMODDBEntries entries;
    std::string dbFilePath;

    static std::string pluginTypeToString(PluginTypeEnum type)
    {
        switch (type) {
        case PluginTypeEnum::Recommended:
            return "Recommended";
        case PluginTypeEnum::Required:
            return "Required";
        case PluginTypeEnum::Optional:
            return "Optional";
        case PluginTypeEnum::NotUsable:
            return "NotUsable";
        case PluginTypeEnum::CouldBeUsable:
            return "CouldBeUsable";
        default:
            return "Optional";
        }
    }

    static std::string fileDependencyStateToString(FileDependencyTypeEnum state)
    {
        switch (state) {
        case FileDependencyTypeEnum::Active:
            return "Active";
        case FileDependencyTypeEnum::Inactive:
            return "Inactive";
        case FileDependencyTypeEnum::Missing:
            return "Missing";
        default:
            return "Missing";
        }
    }

    static StoredDependencies convertCompositeDependency(const CompositeDependency& cd)
    {
        StoredDependencies deps;
        deps.operatorType = (cd.operatorType == OperatorTypeEnum::OR) ? "Or" : "And";

        for (const auto& fd : cd.fileDependencies) {
            deps.fileDependencies.push_back({ fd.file, fileDependencyStateToString(fd.state) });
        }

        for (const auto& fd : cd.flagDependencies) {
            deps.flagDependencies.push_back({ fd.flag, fd.value });
        }

        for (const auto& nd : cd.nestedDependencies) {
            deps.nestedDependencies.push_back(convertCompositeDependency(nd));
        }

        return deps;
    }

    static std::vector<StoredTypePattern> extractTypePatterns(const Plugin& plugin)
    {
        std::vector<StoredTypePattern> patterns;

        // Extract dependency patterns from TypeDescriptor
        for (const auto& pattern : plugin.typeDescriptor.dependencyType.patterns.patterns) {
            StoredTypePattern stored;
            stored.type         = pluginTypeToString(pattern.type);
            stored.dependencies = convertCompositeDependency(pattern.dependencies);
            patterns.push_back(std::move(stored));
        }

        // If no dependency patterns exist but the static type is Recommended/Required,
        // store it as a pattern with empty dependencies (always-true)
        if (patterns.empty() && plugin.typeDescriptor.type != PluginTypeEnum::Optional
            && plugin.typeDescriptor.type != PluginTypeEnum::UNKNOWN) {
            StoredTypePattern fallback;
            fallback.type                 = pluginTypeToString(plugin.typeDescriptor.type);
            fallback.dependencies.operatorType = "And";
            patterns.push_back(std::move(fallback));
        }

        // Also check dependencyType.defaultType as a fallback
        if (patterns.empty() && plugin.typeDescriptor.dependencyType.defaultType.has_value()) {
            auto defaultType = plugin.typeDescriptor.dependencyType.defaultType.value();
            if (defaultType != PluginTypeEnum::Optional && defaultType != PluginTypeEnum::UNKNOWN) {
                StoredTypePattern fallback;
                fallback.type                 = pluginTypeToString(defaultType);
                fallback.dependencies.operatorType = "And";
                patterns.push_back(std::move(fallback));
            }
        }

        return patterns;
    }

    void loadFromFile()
    {
        entries.clear();

        // Create empty file if it doesn't exist
        if (!std::filesystem::exists(dbFilePath)) {
            std::ofstream file(dbFilePath);
            file << "[]"; // Empty JSON array
            file.close();
            return; // No entries to load
        }

        try {
            // Read and parse the JSON file
            std::ifstream file(dbFilePath);
            if (!file.is_open()) {
                return;
            }

            nlohmann::json jsonArray = nlohmann::json::parse(file);

            // Ensure it's an array
            if (!jsonArray.is_array()) {
                return;
            }

            // Process each entry in the array
            for (const auto& entryJson : jsonArray) {
                entries.push_back(std::make_unique<FomodDbEntry>(entryJson));
            }
        } catch ([[maybe_unused]] const std::exception& e) {
            // Handle parsing errors (leave entries empty)
        }
    }
};
