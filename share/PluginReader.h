#ifndef PLUGINREADER_H
#define PLUGINREADER_H

#include <fstream>
#include <vector>
#include <iostream>

#include "stringutil.h"


struct RecordHeader {
    char type[4];  // Record type (e.g., TES4, MAST, etc.)
    uint32_t size; // Record data size
};

inline std::vector<std::string> readMasters(const std::wstring& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open file: " << wstringToString(filename) << std::endl;
        return {};
    }

    std::vector<std::string> masters;
    RecordHeader header;

    // Read the TES4 record
    while (file.read(reinterpret_cast<char*>(&header), sizeof(header))) {
        if (std::string(header.type, 4) == "TES4") {
            std::streampos tes4End = file.tellg() + static_cast<std::streampos>(header.size);

            // Read subrecords inside TES4
            while (file.tellg() < tes4End && file.read(reinterpret_cast<char*>(&header), sizeof(header))) {
                if (std::string(header.type, 4) == "MAST") {
                    std::string masterName(header.size, '\0');
                    file.read(masterName.data(), header.size);
                    masters.push_back(masterName);
                } else {
                    file.seekg(header.size, std::ios::cur); // Skip unknown subrecords
                }
            }
            break; // TES4 is always unique, no need to continue reading
        } else {
            file.seekg(header.size, std::ios::cur); // Skip non-TES4 records
        }
    }

    return masters;
}

#endif //PLUGINREADER_H
