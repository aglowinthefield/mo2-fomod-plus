#ifndef MODULECONFIGURATION_H
#define MODULECONFIGURATION_H

#include <string>
#include <vector>
#include <pugixml.hpp>

class XmlDeserializable {
public:
  virtual bool deserialize(pugi::xml_node& node) = 0;
};

enum class PluginTypeEnum {
    Required,
    Optional,
    Recommended,
    NotUsable,
    CouldBeUsable
};

class PluginType: public XmlDeserializable {
public:
    PluginTypeEnum name;
    bool deserialize(pugi::xml_node& node) override {
      std::string typeStr = node.child("name").text().as_string();
      if (typeStr == "Required") name = PluginTypeEnum::Required;
      else if (typeStr == "Optional") name = PluginTypeEnum::Optional;
      else if (typeStr == "Recommended") name = PluginTypeEnum::Recommended;
      else if (typeStr == "NotUsable") name = PluginTypeEnum::NotUsable;
      else if (typeStr == "CouldBeUsable") name = PluginTypeEnum::CouldBeUsable;
      return true;
    }
};

class FileDependency: public XmlDeserializable {
public:
    std::string file;
    std::string state;
    bool deserialize(pugi::xml_node& node) override {
      file = node.child("file").text().as_string();
      state = node.child("state").text().as_string();
      return true;
    }
};

class FlagDependency: public XmlDeserializable {
public:
    std::string flag;
    std::string value;
    bool deserialize(pugi::xml_node& node) override {
      flag = node.child("flag").text().as_string();
      value = node.child("value").text().as_string();
      return true;
    }
};

class CompositeDependency: public XmlDeserializable {
public:
    std::vector<FileDependency> fileDependencies;
    std::vector<FlagDependency> flagDependencies;
    std::string operatorType;
    bool deserialize(pugi::xml_node& node) override {
      for (pugi::xml_node fileNode : node.children("fileDependency")) {
        FileDependency fileDep;
        fileDep.deserialize(fileNode);
        fileDependencies.push_back(fileDep);
      }
      for (pugi::xml_node flagNode : node.children("flagDependency")) {
        FlagDependency flagDep;
        flagDep.deserialize(flagNode);
        flagDependencies.push_back(flagDep);
      }
      operatorType = node.child("operatorType").text().as_string();
      return true;
    }
};

class DependencyPattern: public XmlDeserializable {
public:
    CompositeDependency dependencies;
    PluginType type;
    bool deserialize(pugi::xml_node& node) override {
      dependencies.deserialize(node.child("dependencies"));
      type.deserialize(node.child("type"));
      return true;
    }
};

class DependencyPatternList: public XmlDeserializable {
public:
    std::vector<DependencyPattern> patterns;
    bool deserialize(pugi::xml_node& node) override {
      for (pugi::xml_node patternNode : node.children("pattern")) {
        DependencyPattern pattern;
        pattern.deserialize(patternNode);
        patterns.push_back(pattern);
      }
      return true;
    }
};

class DependencyPluginType: public XmlDeserializable {
public:
    PluginType defaultType;
    DependencyPatternList patterns;
    bool deserialize(pugi::xml_node& node) override {
      defaultType.deserialize(node.child("defaultType"));
      patterns.deserialize(node.child("patterns"));
      return true;
    }
};

class PluginTypeDescriptor: public XmlDeserializable {
public:
    DependencyPluginType dependencyType;
    PluginType type;
    bool deserialize(pugi::xml_node& node) override {
      dependencyType.deserialize(node.child("dependencyType"));
      type.deserialize(node.child("type"));
      return true;
    }
};

class Image: public XmlDeserializable {
public:
    std::string path;
    bool deserialize(pugi::xml_node& node) override {
      path = node.child("path").text().as_string();
      return true;
    }
};

class HeaderImage : public XmlDeserializable {
public:
    std::string path;
    bool showImage;
    bool showFade;
    int height;

    bool deserialize(pugi::xml_node& node) override {
      path = node.child("path").text().as_string();
      showImage = node.child("showImage").text().as_bool();
      showFade = node.child("showFade").text().as_bool();
      height = node.child("height").text().as_int();
      return true;
    }
};

class Plugin: public XmlDeserializable {
public:
    std::string description;
    Image image;
    PluginTypeDescriptor typeDescriptor;
    std::string name;

    bool deserialize(pugi::xml_node& node) override {
      description = node.child("description").text().as_string();
      image.deserialize(node.child("image"));
      typeDescriptor.deserialize(node.child("typeDescriptor"));
      name = node.child("name").text().as_string();
      return true;
    }
};

class PluginList: public XmlDeserializable {
public:
    std::vector<Plugin> plugins;
    std::string order;
    bool deserialize(pugi::xml_node& node) override {
      for (pugi::xml_node pluginNode : node.children("plugin")) {
        Plugin plugin;
        plugin.deserialize(pluginNode);
        plugins.push_back(plugin);
      }
      order = node.child("order").text().as_string();
      return true;
    }
};

class Group: public XmlDeserializable {
public:
    PluginList plugins;
    std::string name;
    std::string type;
    bool deserialize(pugi::xml_node& node) override {
      plugins.deserialize(node.child("plugins"));
      name = node.child("name").text().as_string();
      type = node.child("type").text().as_string();
      return true;
    }
};

class GroupList: public XmlDeserializable {
public:
    std::vector<Group> groups;
    std::string order;
    bool deserialize(pugi::xml_node& node) override {
      for (pugi::xml_node groupNode : node.children("group")) {
        Group group;
        group.deserialize(groupNode);
        groups.push_back(group);
      }
      order = node.child("order").text().as_string();
      return true;
    }
};

class InstallStep: public XmlDeserializable {
public:
    CompositeDependency visible;
    GroupList optionalFileGroups;
    std::string name;

    bool deserialize(pugi::xml_node& node) override {
      visible.deserialize(node.child("visible"));
      optionalFileGroups.deserialize(node.child("optionalFileGroups"));
      name = node.child("name").text().as_string();
      return true;
    }
};

class StepList: public XmlDeserializable {
public:
    std::vector<InstallStep> installSteps;
    std::string order;

    bool deserialize(pugi::xml_node& node) override {
        for (pugi::xml_node stepNode : node.children("installStep")) {
            InstallStep step;
            step.deserialize(stepNode);
            installSteps.push_back(step);
        }
        order = node.child("order").text().as_string();
        return true;
    }
};

class ModuleConfiguration {
public:
  std::string moduleName;
  HeaderImage moduleImage;
  CompositeDependency moduleDependencies;
  PluginList requiredInstallFiles;
  StepList installSteps;

  bool deserialize(const std::string& filePath);

};



#endif //MODULECONFIGURATION_H
