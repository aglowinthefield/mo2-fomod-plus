#pragma once

#include <fstream>
#include <string>
#include <vector>
#include <cstdint>
#include <unordered_set>

static const std::unordered_set<std::string> VANILLA_MASTERS = {
    "Skyrim.esm",
    "Update.esm",
    "Dawnguard.esm",
    "HearthFires.esm",
    "Dragonborn.esm"
};

class PluginReader {
public:

    /**
     * Reads the master files from a Bethesda plugin file (ESP/ESM/ESL)
     * @param filePath Path to the plugin file
     * @param trimVanilla Exclude the vanilla game masters or not. Mostly to save DB space.
     * @return Vector of master filenames
     */
    static std::vector<std::string> readMasters(const std::string& filePath, const bool trimVanilla = false)
    {
        std::vector<std::string> masters;
        std::ifstream file(filePath, std::ios::binary);

        if (!file) {
            return masters;
        }

        // Check TES4 record signature
        char signature[4];
        file.read(signature, 4);
        if (strncmp(signature, "TES4", 4) != 0) {
            return masters;
        }

        // Read record size
        uint32_t recordSize;
        file.read(reinterpret_cast<char*>(&recordSize), 4);

        constexpr uint32_t skipSize = sizeof(uint32_t) // flags
        + sizeof(uint32_t) // formId
        + sizeof(uint16_t) // timestamp
        + sizeof(uint16_t) // version control
        + sizeof(uint16_t) // internal version
        + sizeof(uint16_t); // unknown

        // Skip header flags, formID, etc. (total 8 bytes)
        file.seekg(skipSize, std::ios::cur);

        // Calculate where the TES4 record ends
        std::streampos recordEnd = file.tellg() + static_cast<std::streampos>(recordSize);

        // Read subrecords until we reach the end of the TES4 record
        while (file && file.tellg() < recordEnd) {
            char subRecordType[4];
            uint16_t subRecordSize;

            // Read subrecord type and size
            file.read(subRecordType, 4);
            file.read(reinterpret_cast<char*>(&subRecordSize), 2);

            if (strncmp(subRecordType, "MAST", 4) == 0) {
                // Read master filename (null-terminated string)
                std::string masterName;
                masterName.resize(subRecordSize);
                file.read(masterName.data(), subRecordSize);

                // Remove null terminator if present
                if (!masterName.empty() && masterName.back() == '\0') {
                    masterName.pop_back();
                }

                // Only add if it's not a vanilla master or if we're not trimming
                if (!trimVanilla || !VANILLA_MASTERS.contains(masterName)) {
                    masters.push_back(masterName);
                }

                // Each MAST is followed by a DATA subrecord
                char dataType[4];
                uint16_t dataSize;
                file.read(dataType, 4);
                file.read(reinterpret_cast<char*>(&dataSize), 2);

                // Skip DATA content (usually an 8-byte value)
                file.seekg(dataSize, std::ios::cur);
            } else {
                // Skip other subrecord types
                file.seekg(subRecordSize, std::ios::cur);
            }
        }

        return masters;
    }

    /**
     * Checks if a file is a valid Bethesda plugin (ESP/ESM/ESL)
     * @param filePath Path to the file
     * @return True if the file is a valid plugin
     */
    static bool isValidPlugin(const std::string& filePath)
    {
        std::ifstream file(filePath, std::ios::binary);

        if (!file) {
            return false;
        }

        char signature[4];
        file.read(signature, 4);

        return strncmp(signature, "TES4", 4) == 0;
    }
};