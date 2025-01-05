#ifndef MODULECONFIGURATION_H
#define MODULECONFIGURATION_H

#include <string>
#include <vector>
#include <pugixml.hpp>
#include <iostream>

class XmlDeserializable {
public:
  virtual ~XmlDeserializable() = default;
  virtual bool deserialize(pugi::xml_node &node) = 0;

protected:
  XmlDeserializable() = default;
};

enum class PluginTypeEnum {
  Required,
  Optional,
  Recommended,
  NotUsable,
  CouldBeUsable
};

class PluginType : public XmlDeserializable {
public:
  PluginTypeEnum name;

  bool deserialize(pugi::xml_node &node) override;
};

class FileDependency : public XmlDeserializable {
public:
  std::string file;
  std::string state;

  bool deserialize(pugi::xml_node &node) override;
};

class FlagDependency : public XmlDeserializable {
public:
  std::string flag;
  std::string value;

  bool deserialize(pugi::xml_node &node) override;
};

class CompositeDependency : public XmlDeserializable {
public:
  std::vector<FileDependency> fileDependencies;
  std::vector<FlagDependency> flagDependencies;
  std::string operatorType;

  bool deserialize(pugi::xml_node &node) override;
};

class DependencyPattern : public XmlDeserializable {
public:
  CompositeDependency dependencies;
  PluginType type;

  bool deserialize(pugi::xml_node &node) override;
};


class DependencyPatternList : public XmlDeserializable {
public:
  std::vector<DependencyPattern> patterns;

  bool deserialize(pugi::xml_node &node) override;
};

class DependencyPluginType : public XmlDeserializable {
public:
  PluginType defaultType;
  DependencyPatternList patterns;

  bool deserialize(pugi::xml_node &node) override;
};

class PluginTypeDescriptor : public XmlDeserializable {
public:
  DependencyPluginType dependencyType;
  PluginType type;

  bool deserialize(pugi::xml_node &node) override;
};

class Image : public XmlDeserializable {
public:
  std::string path;

  bool deserialize(pugi::xml_node &node) override;
};

class HeaderImage : public XmlDeserializable {
public:
  std::string path;
  bool showImage;
  bool showFade;
  int height;

  bool deserialize(pugi::xml_node &node) override;
};

class File : public XmlDeserializable {
public:
  std::string source;
  std::string destination;
  int priority{0};
  bool deserialize(pugi::xml_node &node) override;
};

class FileList : public XmlDeserializable {
public:
  std::vector<File> files;
  bool deserialize(pugi::xml_node &node) override;
};

class Plugin : public XmlDeserializable {
public:
  std::string description;
  Image image;
  PluginTypeDescriptor typeDescriptor;
  std::string name;

  bool deserialize(pugi::xml_node &node) override;
};

class PluginList : public XmlDeserializable {
public:
  std::vector<Plugin> plugins;
  std::string order;

  bool deserialize(pugi::xml_node &node) override;
};

class Group : public XmlDeserializable {
public:
  PluginList plugins;
  std::string name;
  std::string type;

  bool deserialize(pugi::xml_node &node) override;
};

class GroupList : public XmlDeserializable {
public:
  std::vector<Group> groups;
  std::string order;

  bool deserialize(pugi::xml_node &node) override;
};

class InstallStep : public XmlDeserializable {
public:
  DependencyPattern visible;
  GroupList optionalFileGroups;
  std::string name;

  bool deserialize(pugi::xml_node &node) override;
};

class StepList : public XmlDeserializable {
public:
  std::vector<InstallStep> installSteps;
  std::string order;

  bool deserialize(pugi::xml_node &node) override;
};

class ModuleConfiguration {
public:
  std::string moduleName;
  HeaderImage moduleImage;
  CompositeDependency moduleDependencies;
  PluginList requiredInstallFiles;
  StepList installSteps;

  bool deserialize(const std::string &filePath);
};


#endif //MODULECONFIGURATION_H
