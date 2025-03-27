#pragma once

#include <iostream>
#include <optional>
#include <pugixml.hpp>
#include <qstring.h>
#include <string>
#include <vector>

class XmlDeserializable {
public:
    virtual ~XmlDeserializable() = default;

    virtual bool deserialize(pugi::xml_node& node) = 0;

protected:
    XmlDeserializable() = default;
};

enum GroupTypeEnum {
    SelectAny,
    SelectAll,
    SelectExactlyOne,
    SelectAtMostOne,
    SelectAtLeastOne
};

enum class OperatorTypeEnum {
    AND,
    OR
};

enum class OrderTypeEnum {
    Explicit,
    Ascending,
    Descending
};

enum class FileDependencyTypeEnum {
    Missing,
    Inactive,
    Active,
    UNKNOWN_STATE
};

template <typename T>
class OrderedContents {
public:
    OrderTypeEnum order;

    OrderedContents() : order(OrderTypeEnum::Ascending) {}
    explicit OrderedContents(const OrderTypeEnum orderType): order(orderType) {}

    template <typename Accessor>
    bool compare(const T& a, const T& b, Accessor accessor) const
    {
        switch (order) {
        case OrderTypeEnum::Ascending:
            return accessor(a) < accessor(b);
        case OrderTypeEnum::Descending:
            return accessor(a) > accessor(b);
        case OrderTypeEnum::Explicit:
        default:
            return false; // No sorting for explicit order
        }
    }
};

enum class PluginTypeEnum {
    Recommended,
    Required,
    Optional,
    NotUsable,
    CouldBeUsable,
    UNKNOWN
};


inline std::ostream& operator<<(std::ostream& os, const PluginTypeEnum& type)
{
    switch (type) {
    case PluginTypeEnum::Recommended:
        os << "Recommended";
        break;
    case PluginTypeEnum::Required:
        os << "Required";
        break;
    case PluginTypeEnum::Optional:
        os << "Optional";
        break;
    case PluginTypeEnum::NotUsable:
        os << "NotUsable";
        break;
    case PluginTypeEnum::CouldBeUsable:
        os << "CouldBeUsable";
        break;
    default: ;
    }
    return os;
}

class PluginType final : public XmlDeserializable {
public:
    PluginTypeEnum name = PluginTypeEnum::Optional; // sane default

    bool deserialize(pugi::xml_node& node) override;
};

class FileDependency final : public XmlDeserializable {
public:
    std::string file;
    FileDependencyTypeEnum state = FileDependencyTypeEnum::UNKNOWN_STATE;

    bool deserialize(pugi::xml_node& node) override;
};

class FlagDependency final : public XmlDeserializable {
public:
    std::string flag;
    std::string value;

    bool deserialize(pugi::xml_node& node) override;
};

class GameDependency final : public XmlDeserializable {
public:
    std::string version;

    bool deserialize(pugi::xml_node& node) override;
};

class CompositeDependency final : public XmlDeserializable {
public:
    std::vector<FileDependency> fileDependencies;
    std::vector<FlagDependency> flagDependencies;
    std::vector<GameDependency> gameDependencies;
    std::vector<CompositeDependency> nestedDependencies;
    OperatorTypeEnum operatorType = OperatorTypeEnum::AND; // safest default.

    bool deserialize(pugi::xml_node& node) override;
};

class DependencyPattern final : public XmlDeserializable {
public:
    CompositeDependency dependencies;
    PluginTypeEnum type;

    bool deserialize(pugi::xml_node& node) override;
};


class DependencyPatternList final : public XmlDeserializable {
public:
    std::vector<DependencyPattern> patterns;

    bool deserialize(pugi::xml_node& node) override;
};

class DependencyPluginType final : public XmlDeserializable {
public:
    std::optional<PluginTypeEnum> defaultType;
    DependencyPatternList patterns;

    bool deserialize(pugi::xml_node& node) override;
};

class TypeDescriptor final : public XmlDeserializable {
public:
    DependencyPluginType dependencyType;
    PluginTypeEnum type;

    bool deserialize(pugi::xml_node& node) override;
};

class Image final : public XmlDeserializable {
public:
    std::string path;

    bool deserialize(pugi::xml_node& node) override;
};

class HeaderImage final : public XmlDeserializable {
public:
    std::string path;
    bool showImage;
    bool showFade;
    int height;

    bool deserialize(pugi::xml_node& node) override;
};

class File final : public XmlDeserializable {
public:
    std::string source;
    std::optional<std::string> destination;
    int priority{ 0 };
    bool isFolder;

    bool deserialize(pugi::xml_node& node) override;
};


class FileList final : public XmlDeserializable {
public:
    std::vector<File> files;

    bool deserialize(pugi::xml_node& node) override;
};

class ConditionalFileInstallPattern final : public XmlDeserializable {
public:
    CompositeDependency dependencies;
    FileList files;

    bool deserialize(pugi::xml_node& node) override;
};

// <flag name="2">On</flag>
class ConditionFlag final : public XmlDeserializable {
public:
    std::string name;
    std::string value;

    bool deserialize(pugi::xml_node& node) override;
};

class ConditionFlagList final : public XmlDeserializable {
public:
    std::vector<ConditionFlag> flags;

    bool deserialize(pugi::xml_node& node) override;
};

class Plugin final : public XmlDeserializable {
public:
    std::string description;
    Image image;
    TypeDescriptor typeDescriptor;
    std::string name;
    ConditionFlagList conditionFlags;
    FileList files;

    bool deserialize(pugi::xml_node& node) override;
};

class PluginList final : public XmlDeserializable, public OrderedContents<Plugin> {
public:
    std::vector<Plugin> plugins;
    OrderTypeEnum order;

    bool deserialize(pugi::xml_node& node) override;
};

class Group final : public XmlDeserializable {
public:
    PluginList plugins;
    std::string name;
    GroupTypeEnum type;

    bool deserialize(pugi::xml_node& node) override;
};

class GroupList final : public XmlDeserializable, public OrderedContents<Group> {
public:
    std::vector<Group> groups;
    OrderTypeEnum order;

    bool deserialize(pugi::xml_node& node) override;
};

class InstallStep final : public XmlDeserializable {
public:
    CompositeDependency visible;
    GroupList optionalFileGroups;
    std::string name;

    bool deserialize(pugi::xml_node& node) override;
};

class ConditionalFileInstall final : public XmlDeserializable {
public:
    std::vector<ConditionalFileInstallPattern> patterns;

    bool deserialize(pugi::xml_node& node) override;
};

class StepList final : public XmlDeserializable, public OrderedContents<InstallStep> {
public:
    std::vector<InstallStep> installSteps;
    OrderTypeEnum order;

    bool deserialize(pugi::xml_node& node) override;
};

class ModuleConfiguration {
public:
    std::string moduleName;
    HeaderImage moduleImage;
    CompositeDependency moduleDependencies;
    FileList requiredInstallFiles;
    StepList installSteps;
    ConditionalFileInstall conditionalFileInstalls;

    bool deserialize(const QString& filePath);
};