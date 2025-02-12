#include "FomodInfoFile.h"
#include "XmlParseException.h"
#include <format>
#include <pugixml.hpp>

#include "stringutil.h"

#include <QString>

bool FomodInfoFile::deserialize(const QString& filePath)
{
    pugi::xml_document doc;
    // ReSharper disable once CppTooWideScopeInitStatement
    const pugi::xml_parse_result result = doc.load_file(filePath.toStdWString().c_str());

    if (!result) {
        throw XmlParseException(std::format("XML parsed with errors: {}", result.description()));
    }

    const pugi::xml_node fomodNode = doc.child("fomod");
    if (!fomodNode) {
        throw XmlParseException("No <config> node found");
    }

    name        = fomodNode.child("Name").text().as_string();
    author      = fomodNode.child("Author").text().as_string();
    version     = fomodNode.child("Version").text().as_string();
    website     = fomodNode.child("Website").text().as_string();
    description = fomodNode.child("Description").text().as_string();

    trim({ name, author, version, website, description });

    for (pugi::xml_node groupNode : fomodNode.child("Groups").children("element")) {
        groups.emplace_back(groupNode.text().as_string());
    }

    return true;

}