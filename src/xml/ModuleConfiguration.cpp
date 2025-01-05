#include "ModuleConfiguration.h"

#include <format>

#include "XmlParseException.h";


bool PluginType::deserialize(pugi::xml_node &node) {
  const std::string typeStr = node.attribute("name").as_string();
  if (typeStr == "Required")           name = PluginTypeEnum::Required;
  else if (typeStr == "Optional")      name = PluginTypeEnum::Optional;
  else if (typeStr == "Recommended")   name = PluginTypeEnum::Recommended;
  else if (typeStr == "NotUsable")     name = PluginTypeEnum::NotUsable;
  else if (typeStr == "CouldBeUsable") name = PluginTypeEnum::CouldBeUsable;
  return true;
}

bool FileDependency::deserialize(pugi::xml_node &node) {
  file = node.attribute("file").as_string();
  state = node.attribute("state").as_string();
  return true;
}

bool FlagDependency::deserialize(pugi::xml_node &node) {
  flag = node.attribute("flag").as_string();
  value = node.attribute("value").as_string();
  return true;
}

bool CompositeDependency::deserialize(pugi::xml_node &node) {
  for (pugi::xml_node fileNode: node.children("fileDependency")) {
    FileDependency fileDep;
    fileDep.deserialize(fileNode);
    fileDependencies.push_back(fileDep);
  }
  for (pugi::xml_node flagNode: node.children("flagDependency")) {
    FlagDependency flagDep;
    flagDep.deserialize(flagNode);
    flagDependencies.push_back(flagDep);
  }

  const std::string operatorStr = node.attribute("operator").as_string();
  if (operatorStr == "And") operatorType = OperatorTypeEnum::AND;
  else if (operatorStr == "Or") operatorType = OperatorTypeEnum::OR;

  return true;
}

bool DependencyPattern::deserialize(pugi::xml_node &node) {
  if (!node) return false;
  pugi::xml_node dependenciesNode = node.child("dependencies");
  pugi::xml_node typeNode = node.child("type");
  dependencies.deserialize(dependenciesNode);
  type.deserialize(typeNode);
  return true;
}

bool DependencyPatternList::deserialize(pugi::xml_node &node) {
  for (pugi::xml_node patternNode: node.children("pattern")) {
    DependencyPattern pattern;
    pattern.deserialize(patternNode);
    patterns.push_back(pattern);
  }
  return true;
}

bool DependencyPluginType::deserialize(pugi::xml_node &node) {
  pugi::xml_node defaultTypeNode = node.child("defaultType");
  pugi::xml_node patternsNode = node.child("patterns");
  defaultType.deserialize(defaultTypeNode);
  patterns.deserialize(patternsNode);
  return true;
}

bool PluginTypeDescriptor::deserialize(pugi::xml_node &node) {
  pugi::xml_node dependencyTypeNode = node.child("dependencyType");
  pugi::xml_node typeNode = node.child("type");

  dependencyType.deserialize(dependencyTypeNode);
  type.deserialize(typeNode);
  return true;
}

bool Image::deserialize(pugi::xml_node &node) {
  path = node.attribute("path").as_string();
  return true;
}

bool HeaderImage::deserialize(pugi::xml_node &node) {
  path = node.attribute("path").as_string();
  showImage = node.attribute("showImage").as_bool();
  showFade = node.attribute("showFade").as_bool();
  height = node.attribute("height").as_int();
  return true;
}

bool FileList::deserialize(pugi::xml_node &node) {
  for (pugi::xml_node fileNode: node.children("file")) {
    File file;
    file.deserialize(fileNode);
    files.push_back(file);
  }
  return true;
}

bool File::deserialize(pugi::xml_node &node) {
  source = node.attribute("source").as_string();
  destination = node.attribute("destination").as_string();
  priority = node.attribute("priority").as_int();
  return true;
}


bool Plugin::deserialize(pugi::xml_node &node) {
  pugi::xml_node imageNode = node.child("image");
  pugi::xml_node typeDescriptorNode = node.child("typeDescriptor");

  description = node.child("description").text().as_string();
  image.deserialize(imageNode);
  typeDescriptor.deserialize(typeDescriptorNode);
  name = node.attribute("name").as_string();
  return true;
}

bool PluginList::deserialize(pugi::xml_node &node) {
  for (pugi::xml_node pluginNode: node.children("plugin")) {
    Plugin plugin;
    plugin.deserialize(pluginNode);
    plugins.push_back(plugin);
  }
  order = node.attribute("order").as_string();
  return true;
}

bool Group::deserialize(pugi::xml_node &node) {
  pugi::xml_node pluginsNode = node.child("plugins");
  plugins.deserialize(pluginsNode);
  name = node.attribute("name").as_string();
  type = node.attribute("type").as_string();
  return true;
}

bool GroupList::deserialize(pugi::xml_node &node) {
  for (pugi::xml_node groupNode: node.children("group")) {
    Group group;
    group.deserialize(groupNode);
    groups.push_back(group);
  }
  order = node.attribute("order").as_string();
  return true;
}

bool InstallStep::deserialize(pugi::xml_node &node) {
  pugi::xml_node visibleNode = node.child("visible");
  pugi::xml_node optionalFileGroupsNode = node.child("optionalFileGroups");
  visible.deserialize(visibleNode);
  optionalFileGroups.deserialize(optionalFileGroupsNode);
  name = node.attribute("name").as_string();
  return true;
}

bool StepList::deserialize(pugi::xml_node &node) {
  for (pugi::xml_node stepNode: node.children("installStep")) {
    InstallStep step;
    step.deserialize(stepNode);
    installSteps.push_back(step);
  }
  order = node.attribute("order").as_string();
  return true;
}

bool ModuleConfiguration::deserialize(const std::string &filePath) {
  pugi::xml_document doc;

  if (const pugi::xml_parse_result result = doc.load_file(filePath.c_str()); !result) {
    throw XmlParseException(std::format("XML parsed with errors: {}", result.description()));
  }

  const pugi::xml_node configNode = doc.child("config");
  if (!configNode) {
    throw XmlParseException("No <config> node found");
  }

  moduleName = configNode.child("moduleName").text().as_string();

  moduleImage = HeaderImage();
  pugi::xml_node moduleImageNode = configNode.child("moduleImage");
  moduleImage.deserialize(moduleImageNode);

  pugi::xml_node moduleDependenciesNode = configNode.child("moduleDependencies");
  moduleDependencies.deserialize(moduleDependenciesNode);

  pugi::xml_node requiredInstallFilesNode = configNode.child("requiredInstallFiles");
  requiredInstallFiles.deserialize(requiredInstallFilesNode);

  pugi::xml_node installStepsNode = configNode.child("installSteps");
  installSteps.deserialize(installStepsNode);

  return true;
}
