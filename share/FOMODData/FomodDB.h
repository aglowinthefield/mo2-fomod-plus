#pragma once

#include <fstream>
#include <stringutil.h>

#include "FomodDBEntry.h"

#include <xml/ModuleConfiguration.h>

#include "PluginReader.h"

using FOMODDBEntries = std::vector<std::shared_ptr<FomodDbEntry> >;

constexpr std::string FOMOD_DB_FILE = "fomod.db";

class FomodDB {
public:
  /**
   *
   * @param moBasePath The organizer instance's basePath() value
   * @param dbName The filename of the db. Only settable for testing purposes.
   */
  explicit FomodDB(const std::string &moBasePath, const std::string &dbName = FOMOD_DB_FILE) {
    dbFilePath = (std::filesystem::path(moBasePath) / dbName).string();
    loadFromFile();
  }

  // TODO: Also pull from non install steps (requiredInstallFiles or whatever, and optional);
  static std::shared_ptr<FomodDbEntry> getEntryFromFomod(ModuleConfiguration *fomod, std::vector<QString> pluginPaths,
                                                  int modId) {
    std::vector<FomodOption> options;
    for (const auto &installStep: fomod->installSteps.installSteps) {
      for (const auto &group: installStep.optionalFileGroups.groups) {
        for (const auto &plugin: group.plugins.plugins) {
          // Create a DB entry for the given plugin if it has an ESP
          std::cout << "\nPlugin: " << plugin.name << std::endl;

          for (auto file: plugin.files.files) {
            if (file.isFolder || !isPluginFile(file.source)) {
              continue;
            }

            // Find the path in pluginPaths that ends with this path
            // PluginPaths is gathered from the archive contents.
            auto it = std::ranges::find_if(pluginPaths, [&file](const QString &path) {
              return path.endsWith(file.source.c_str());
            });
            if (it == pluginPaths.end()) {
              continue;
            }
            const auto &pluginPath = *it;
            const auto masters = PluginReader::readMasters(pluginPath.toStdString(), true);
            options.emplace_back(
              plugin.name,
              file.source,
              masters,
              installStep.name,
              group.name
            );
          }
        }
      }
    }
    return std::make_shared<FomodDbEntry>(modId, fomod->moduleName, options);
  }

  void addEntry(const std::shared_ptr<FomodDbEntry> &entry, const bool upsert = true) {
    // TODO: Test this upsert.
    if (upsert) {
      const auto it = std::ranges::find_if(entries, [&entry](const std::shared_ptr<FomodDbEntry> &e) {
        return e->getModId() == entry->getModId();
      });
      if (it != entries.end()) {
        *it = entry;
      } else {
        entries.emplace_back(entry);
      }
    } else {
      entries.emplace_back(entry);
    }
  }

  [[nodiscard]] const FOMODDBEntries &getEntries() { return entries; }

  void saveToFile() const {
    try {
      std::ofstream file(dbFilePath);
      if (!file.is_open()) {
        return;
      }

      file << toJson().dump(2); // Pretty-print with 2-space indentation
      file.close();
    } catch ([[maybe_unused]] const std::exception &e) {
      // Handle saving errors
    }
  }

  [[nodiscard]] nlohmann::json toJson() const {
    nlohmann::json jsonArray = nlohmann::json::array();

    for (const auto &entry: entries) {
      jsonArray.push_back(entry->toJson());
    }

    return jsonArray;
  }

private:
  FOMODDBEntries entries;
  std::string dbFilePath;

  void loadFromFile() {
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
      for (const auto &entryJson: jsonArray) {
        entries.push_back(std::make_unique<FomodDbEntry>(entryJson));
      }
    } catch ([[maybe_unused]] const std::exception &e) {
      // Handle parsing errors (leave entries empty)
    }
  }
};
