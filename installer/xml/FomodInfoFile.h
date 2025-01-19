#ifndef FOMODINFOFILE_H
#define FOMODINFOFILE_H

#include <string>
#include <vector>

class FomodInfoFile {
public:
    bool deserialize(const std::string& filePath);

    [[nodiscard]] const std::string& getName() const { return name; }
    [[nodiscard]] const std::string& getAuthor() const { return author; }
    [[nodiscard]] const std::string& getVersion() const { return version; }
    [[nodiscard]] const std::string& getWebsite() const { return website; }
    [[nodiscard]] const std::string& getDescription() const { return description; }
    [[nodiscard]] const std::vector<std::string>& getGroups() const { return groups; }

private:
    std::string name;
    std::string author;
    std::string version;
    std::string website;
    std::string description;
    std::vector<std::string> groups;
};

#endif //FOMODINFOFILE_H