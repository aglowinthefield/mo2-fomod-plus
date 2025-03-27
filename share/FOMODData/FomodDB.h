#pragma once

#include <fstream>

#include "FomodDBEntry.h"

#include <xml/ModuleConfiguration.h>

using FOMODDBEntries = std::vector<std::unique_ptr<FomodDbEntry> >;

constexpr std::string FOMOD_DB_FILE = "fomod.db";

class FomodDB {
public:
  /**
   *
   * @param moBasePath The organizer instance's basePath() value
   * @param dbName The filename of the db. Only settable for testing purposes.
   */
  explicit FomodDB(const std::string &moBasePath, const std::string& dbName = FOMOD_DB_FILE) {
    dbFilePath = (std::filesystem::path(moBasePath) / dbName).string();
    loadFromFile();
  }

  void addEntriesFromFomod(std::unique_ptr<ModuleConfiguration> fomod) {
    for (auto installStep: fomod->installSteps.installSteps) {
      for (auto group: installStep.optionalFileGroups.groups) {
        for (auto plugin: group.plugins.plugins) {
          // Create a DB entry for the given plugin if it has an ESP
          std::cout << "Plugin: " << plugin.name << std::endl;
        }
      }
    }
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
