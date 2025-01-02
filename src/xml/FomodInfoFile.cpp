#include "FomodInfoFile.h"
#include <pugixml.hpp>

bool FomodInfoFile::deserialize(const std::string &filePath) {
    pugi::xml_document doc;
    // ReSharper disable once CppTooWideScopeInitStatement
    const pugi::xml_parse_result result = doc.load_file(filePath.c_str());

    if (!result) {
        std::cerr << "XML parsed with errors: " << result.description() << std::endl;
        return false;
    }

    const pugi::xml_node fomodNode = doc.child("fomod");
    if (!fomodNode) {
        std::cerr << "No <fomod> node found" << std::endl;
        return false;
    }

    name = fomodNode.child("Name").text().as_string();
    author = fomodNode.child("Author").text().as_string();
    version = fomodNode.child("Version").text().as_string();
    website = fomodNode.child("Website").text().as_string();
    description = fomodNode.child("Description").text().as_string();

    for (pugi::xml_node groupNode : fomodNode.child("Groups").children("element")) {
        groups.emplace_back(groupNode.text().as_string());
    }

    return true;

}

