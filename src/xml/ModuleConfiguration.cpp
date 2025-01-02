#include "ModuleConfiguration.h"
#include <pugixml.hpp>
#include <iostream>

bool ModuleConfiguration::deserialize(const std::string& filePath) {
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(filePath.c_str());

    if (!result) {
        std::cerr << "XML parsed with errors: " << result.description() << std::endl;
        return false;
    }

    pugi::xml_node configNode = doc.child("config");
    if (!configNode) {
        std::cerr << "No <config> node found" << std::endl;
        return false;
    }

    moduleName = configNode.child("moduleName").text().as_string();
    // Deserialize other fields similarly...

    return true;
}
