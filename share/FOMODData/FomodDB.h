#pragma once

#include <fstream>
#include <stringutil.h>

#include "FomodDBEntry.h"

#include <xml/ModuleConfiguration.h>

#include "PluginReader.h"

using FOMODDBEntries = std::vector<std::unique_ptr<FomodDbEntry> >;

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

  FOMODDBEntries getEntriesFromFomod(ModuleConfiguration* fomod, std::vector<QString> pluginPaths) {
    FOMODDBEntries entries;
    for (auto installStep: fomod->installSteps.installSteps) {
      for (auto group: installStep.optionalFileGroups.groups) {
        for (auto plugin: group.plugins.plugins) {
          // Create a DB entry for the given plugin if it has an ESP
          std::cout << "\nPlugin: " << plugin.name << std::endl;
          for (auto file: plugin.files.files) {
            if (file.isFolder) {
              continue;
            }
            if (!isPluginFile(file.source)) {
              continue;
            }

            // Find the path in pluginPaths that ends with this path
            auto it = std::find_if(pluginPaths.begin(), pluginPaths.end(), [&file](const QString &path) {
              return path.endsWith(file.source.c_str());
            });
            if (it == pluginPaths.end()) {
              continue;
            }
            const auto pluginPath = *it;

            std::cout << "File: " << file.source << std::endl;
            std::cout << "Plugin path: " << pluginPath.toStdString() << std::endl;

            const auto masters = PluginReader::readMasters(pluginPath.toStdString());
            for (auto master : masters) {
              std::cout << "Master: " << master << std::endl;
            }
          }
        }
      }
    }
    return entries;
  }

  [[nodiscard]] const FOMODDBEntries &getEntries() { return entries; }

private:
  void addEntry(std::unique_ptr<FomodDbEntry> entry) {
    entries.push_back(std::move(entry));
  }

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


  FOMODDBEntries entries;
  std::string dbFilePath;
};
