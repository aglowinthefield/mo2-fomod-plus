#include "ModuleConfiguration.h"

#include <format>

#include "XmlHelper.h"
#include "XmlParseException.h"
#include "stringutil.h"

using namespace StringConstants::FomodFiles;

static GroupTypeEnum groupTypeFromString(const std::string& groupType)
{
    if (groupType == "SelectAny")
        return SelectAny;
    if (groupType == "SelectAll")
        return SelectAll;
    if (groupType == "SelectExactlyOne")
        return SelectExactlyOne;
    if (groupType == "SelectAtMostOne")
        return SelectAtMostOne;
    if (groupType == "SelectAtLeastOne")
        return SelectAtLeastOne;
    return SelectAny; // is this a sane default? probably
}

PluginTypeEnum pluginTypeFromString(const std::string& typeStr)
{
    if (typeStr == TYPE_REQUIRED)
        return PluginTypeEnum::Required;
    if (typeStr == TYPE_OPTIONAL)
        return PluginTypeEnum::Optional;
    if (typeStr == TYPE_RECOMMENDED)
        return PluginTypeEnum::Recommended;
    if (typeStr == TYPE_NOT_USABLE)
        return PluginTypeEnum::NotUsable;
    if (typeStr == TYPE_COULD_BE_USABLE)
        return PluginTypeEnum::CouldBeUsable;
    return PluginTypeEnum::Optional;
}

template <typename T>
bool deserializeList(pugi::xml_node& node, const char* childName, std::vector<T>& list)
{
    for (pugi::xml_node childNode : node.children(childName)) {
        T item;
        item.deserialize(childNode);
        list.push_back(item);
    }
    return true;
}

/*
 * NOTE: We call 'trim()' on all error-prone fields that rely on user-input. I'm assuming these FOMODs are created with
 * the FOMOD creation tool, so I won't trim things like flags that the tool sets for the user.
 */

bool FileDependency::deserialize(pugi::xml_node& node)
{
    file = node.attribute("file").as_string();
    trim(file);
    // ReSharper disable once CppTooWideScopeInitStatement
    // Do not use 'auto' for these. it will break equality checks
    const std::string stateStr = node.attribute("state").as_string();

    if (stateStr == "Missing")
        state = FileDependencyTypeEnum::Missing;
    else if (stateStr == "Inactive")
        state = FileDependencyTypeEnum::Inactive;
    else if (stateStr == "Active")
        state = FileDependencyTypeEnum::Active;
    return true;
}

bool FlagDependency::deserialize(pugi::xml_node& node)
{
    flag  = node.attribute("flag").as_string();
    value = node.attribute("value").as_string();
    trim({ flag, value });
    return true;
}

bool GameDependency::deserialize(pugi::xml_node& node)
{
    version = node.attribute("version").as_string();
    trim(version);
    return true;
}

bool CompositeDependency::deserialize(pugi::xml_node& node)
{

    // this could EITHER have a dependencies child or the dependencies are here.
    // turns out they could have both.
    pugi::xml_node possibleNode = node;

    // If the dependencies are all right inside, just use the root node as the dependency base.
    // This looks hacky but accommodates both _nested_ dependencies for plugins, and extremely simple ones for step visibility.
    if (node.child("dependencies") && !node.child("fileDependency") && !node.child("flagDependency") &&!node.child("gameDependency")) {
        possibleNode = node.child("dependencies");
    }

    deserializeList(possibleNode, "fileDependency", fileDependencies);
    deserializeList(possibleNode, "flagDependency", flagDependencies);
    deserializeList(possibleNode, "gameDependency", gameDependencies);
    deserializeList(possibleNode, "dependencies", nestedDependencies);

    operatorType = OperatorTypeEnum::AND; // safest default.

    if (const std::string operatorStr = possibleNode.attribute("operator").as_string(); operatorStr == "Or") {
        operatorType = OperatorTypeEnum::OR;
    }

    totalDependencies = static_cast<int>(fileDependencies.size() + flagDependencies.size() + gameDependencies.size());
    return true;
}

bool DependencyPattern::deserialize(pugi::xml_node& node)
{
    if (!node)
        return false;
    pugi::xml_node dependenciesNode = node.child("dependencies");
    dependencies.deserialize(dependenciesNode);

    const pugi::xml_node typeNode = node.child("type");
    type                          = pluginTypeFromString(typeNode.attribute("name").as_string());
    return true;
}

bool DependencyPatternList::deserialize(pugi::xml_node& node)
{
    return deserializeList(node, "pattern", patterns);
}

bool DependencyPluginType::deserialize(pugi::xml_node& node)
{
    pugi::xml_node patternsNode          = node.child("patterns");
    const pugi::xml_node defaultTypeNode = node.child("defaultType");
    defaultType                          = pluginTypeFromString(defaultTypeNode.attribute("name").as_string());
    patterns.deserialize(patternsNode);
    return true;
}

bool TypeDescriptor::deserialize(pugi::xml_node& node)
{
    pugi::xml_node dependencyTypeNode = node.child("dependencyType");
    dependencyType.deserialize(dependencyTypeNode);
    const pugi::xml_node typeNode = node.child("type");
    type                          = pluginTypeFromString(typeNode.attribute("name").as_string());
    return true;
}

bool Image::deserialize(pugi::xml_node& node)
{
    path = node.attribute("path").as_string();
    return true;
}

bool HeaderImage::deserialize(pugi::xml_node& node)
{
    path      = node.attribute("path").as_string();
    showImage = node.attribute("showImage").as_bool();
    showFade  = node.attribute("showFade").as_bool();
    height    = node.attribute("height").as_int();
    return true;
}

bool FileList::deserialize(pugi::xml_node& node)
{
    for (pugi::xml_node childNode : node.children()) {
        if (std::string(childNode.name()) == "folder"
            || std::string(childNode.name()) == "file") {
            File file;
            file.deserialize(childNode);
            files.emplace_back(file);
        }
    }
    return true;
}

bool ConditionalFileInstallPattern::deserialize(pugi::xml_node& node)
{
    pugi::xml_node dependenciesNode = node.child("dependencies");
    pugi::xml_node filesNode        = node.child("files");

    dependencies.deserialize(dependenciesNode);
    files.deserialize(filesNode);

    return true;
}

// <flag name="2">On</flag>
bool ConditionFlag::deserialize(pugi::xml_node& node)
{
    name  = node.attribute("name").as_string();
    value = node.child_value(); //
    return true;
}

bool ConditionFlagList::deserialize(pugi::xml_node& node)
{
    return deserializeList(node, "flag", flags);
}

bool File::deserialize(pugi::xml_node& node)
{
    source      = node.attribute("source").as_string();
    // destination = node.attribute("destination").as_string();
    priority    = node.attribute("priority").as_int();
    isFolder    = strcmp(node.name(), "folder") == 0;
    if (auto attr = node.attribute("destination"); attr) {
        destination = attr.as_string();
    } else {
        destination = std::nullopt;
    }
    return true;
}


bool Plugin::deserialize(pugi::xml_node& node)
{
    pugi::xml_node imageNode          = node.child("image");
    pugi::xml_node typeDescriptorNode = node.child("typeDescriptor");
    pugi::xml_node conditionFlagsNode = node.child("conditionFlags");
    pugi::xml_node filesNode          = node.child("files");

    description = node.child("description").text().as_string();
    description = trim(description); // Find a better way to do this eventually.
    image.deserialize(imageNode);
    typeDescriptor.deserialize(typeDescriptorNode);
    name = node.attribute("name").as_string();
    name = trim(name);
    conditionFlags.deserialize(conditionFlagsNode);
    files.deserialize(filesNode);
    return true;
}

bool PluginList::deserialize(pugi::xml_node& node)
{
    deserializeList(node, "plugin", plugins);
    order = XmlHelper::getOrderType(node.attribute("order").as_string(), OrderTypeEnum::Ascending);

    // Sort the plugins based on the specified order
    std::ranges::sort(plugins, [this](const Plugin& a, const Plugin& b) {
        if (order == OrderTypeEnum::Ascending) {
            return a.name < b.name;
        }
        if (order == OrderTypeEnum::Descending) {
            return a.name > b.name;
        }
        return false; // Default case, no sorting
    });

    return true;
}

bool Group::deserialize(pugi::xml_node& node)
{
    pugi::xml_node pluginsNode = node.child("plugins");
    plugins.deserialize(pluginsNode);
    name = node.attribute("name").as_string();
    type = groupTypeFromString(node.attribute("type").as_string());
    return true;
}

bool GroupList::deserialize(pugi::xml_node& node)
{
    deserializeList(node, "group", groups);
    order = XmlHelper::getOrderType(node.attribute("order").as_string());

    // Sort the groups based on the specified order
    std::ranges::sort(groups, [this](const Group& a, const Group& b) {
        if (order == OrderTypeEnum::Ascending) {
            return a.name < b.name;
        }
        if (order == OrderTypeEnum::Descending) {
            return a.name > b.name;
        }
        return false; // Default case, no sorting
    });
    return true;
}

bool InstallStep::deserialize(pugi::xml_node& node)
{
    pugi::xml_node visibleNode            = node.child("visible");
    pugi::xml_node optionalFileGroupsNode = node.child("optionalFileGroups");
    visible.deserialize(visibleNode);
    optionalFileGroups.deserialize(optionalFileGroupsNode);
    name = node.attribute("name").as_string();
    return true;
}

bool ConditionalFileInstall::deserialize(pugi::xml_node& node)
{
    pugi::xml_node patternsNode = node.child("patterns");
    deserializeList(patternsNode, "pattern", patterns);
    return true;
}

bool StepList::deserialize(pugi::xml_node& node)
{
    deserializeList(node, "installStep", installSteps);
    order = XmlHelper::getOrderType(node.attribute("order").as_string());
    return true;
}

bool ModuleConfiguration::deserialize(const std::string& filePath)
{
    pugi::xml_document doc_;

    if (const pugi::xml_parse_result result = doc_.load_file(filePath.c_str()); !result) {
        throw XmlParseException(std::format("XML parsed with errors: {}", result.description()));
    }

    pugi::xml_document doc;
    doc.reset(doc_);

    const pugi::xml_node configNode = doc_.child("config");
    if (!configNode) {
        throw XmlParseException("No <config> node found");
    }

    moduleName = configNode.child("moduleName").text().as_string();

    moduleImage                    = HeaderImage();
    pugi::xml_node moduleImageNode = configNode.child("moduleImage");
    moduleImage.deserialize(moduleImageNode);

    pugi::xml_node moduleDependenciesNode = configNode.child("moduleDependencies");
    moduleDependencies.deserialize(moduleDependenciesNode);

    pugi::xml_node requiredInstallFilesNode = configNode.child("requiredInstallFiles");
    requiredInstallFiles.deserialize(requiredInstallFilesNode);

    pugi::xml_node installStepsNode = configNode.child("installSteps");
    installSteps.deserialize(installStepsNode);

    pugi::xml_node conditionalFileInstallsNode = configNode.child("conditionalFileInstalls");
    conditionalFileInstalls.deserialize(conditionalFileInstallsNode);

    return true;
}