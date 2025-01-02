#ifndef FOMODINFOFILE_H
#define FOMODINFOFILE_H

#include <string>
#include <vector>
#include <iostream>

class FomodInfoFile {
public:
    bool deserialize(const std::string& filePath);
    const std::string& getName() const { return name; }
    const std::string& getAuthor() const { return author; }
    const std::string& getVersion() const { return version; }
    const std::string& getWebsite() const { return website; }
    const std::string& getDescription() const { return description; }
    const std::vector<std::string>& getGroups() const { return groups; }

private:
    std::string name;
    std::string author;
    std::string version;
    std::string website;
    std::string description;
    std::vector<std::string> groups;
};

#endif //FOMODINFOFILE_H
