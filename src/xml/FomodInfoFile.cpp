#include "FomodInfoFile.h"
#include <log.h>
#include <pugixml.hpp>

using namespace MOBase;

bool FomodInfoFile::deserialize(const std::string &filePath) {
    pugi::xml_document doc;
    // ReSharper disable once CppTooWideScopeInitStatement
    const pugi::xml_parse_result result = doc.load_file(filePath.c_str());

    if (!result) {
        log::error("XML parsed with errors: {}", result.description());
        return false;
    }

    const pugi::xml_node fomodNode = doc.child("fomod");
    if (!fomodNode) {
        log::error("No <config> node found");
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

