#pragma once

#include <fstream>

#include "FomodDBEntry.h"

using FOMODDBEntries = std::vector<std::unique_ptr<FomodDbEntry> >;

constexpr std::string FOMOD_DB_FILE = "fomod.db";

class FomodDB {
public:
  /**
   *
   * @param moBasePath The organizer instance's basePath() value
   */
  explicit FomodDB(const std::string &moBasePath) {
    dbFilePath = (std::filesystem::path(moBasePath) / FOMOD_DB_FILE).string();
    loadFromFile();
  }

  [[nodiscard]] const FOMODDBEntries &getEntries() { return entries; }

private:
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
