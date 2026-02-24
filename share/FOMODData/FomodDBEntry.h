#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <utility>
#include <vector>

/*
The following JSON will be part of an array of similar objects in the root level "JSON DB" for FOMOD Plus.
It contains information to resolve the identity of a given mod (names can change), and then the options
in the FOMOD with their respective masters.
{
    modId: 12345,
    displayName: "Lux (Patch Hub)",
    options: [
        {
            "name": "JK's The Hag's Cure",
            "fileName": "Lux - JK's The Hag's Cure patch.esp",
            "masters": [
                "Skyrim.esm",
                "JK's The Hag's Cure.esp",
                "Lux - Resources.esp",
                "Lux.esp"
            ],
            "step": "Page One",
            "group": "Group One",
            "selectionState": "Available",
            "typePatterns": [
                {
                    "type": "Recommended",
                    "dependencies": {
                        "operator": "Or",
                        "fileDependencies": [
                            { "file": "TKDodge.esp", "state": "Active" }
                        ],
                        "flagDependencies": [
                            { "flag": "SomeFlag", "value": "On" }
                        ],
                        "nestedDependencies": []
                    }
                }
            ]
        }
    ]
}
*/

enum class SelectionState {
    Unknown, // Not yet matched to choices
    Selected, // User selected this option
    Deselected, // User manually deselected
    Available // Present but user didn't interact (or choices not recorded)
};

inline std::string selectionStateToString(SelectionState state)
{
    switch (state) {
    case SelectionState::Unknown:
        return "Unknown";
    case SelectionState::Selected:
        return "Selected";
    case SelectionState::Deselected:
        return "Deselected";
    case SelectionState::Available:
        return "Available";
    default:
        return "Unknown";
    }
}

inline SelectionState stringToSelectionState(const std::string& str)
{
    if (str == "Selected")
        return SelectionState::Selected;
    if (str == "Deselected")
        return SelectionState::Deselected;
    if (str == "Available")
        return SelectionState::Available;
    return SelectionState::Unknown;
}

// Stored version of FileDependency (XML-independent, JSON-serializable)
struct StoredFileDependency {
    std::string file;
    std::string state; // "Active", "Inactive", "Missing"
};

// Stored version of FlagDependency (for completeness; not evaluable in PatchFinder)
struct StoredFlagDependency {
    std::string flag;
    std::string value;
};

// Stored version of CompositeDependency (recursive AND/OR composition)
struct StoredDependencies {
    std::string operatorType; // "And", "Or"
    std::vector<StoredFileDependency> fileDependencies;
    std::vector<StoredFlagDependency> flagDependencies;
    std::vector<StoredDependencies> nestedDependencies;
};

// One condition→type pattern from TypeDescriptor
struct StoredTypePattern {
    std::string type; // "Recommended", "Required", "Optional", "NotUsable", etc.
    StoredDependencies dependencies;
};

// --- JSON serialization helpers ---

inline nlohmann::json storedDependenciesToJson(const StoredDependencies& deps)
{
    nlohmann::json j;
    j["operator"] = deps.operatorType;

    nlohmann::json fileArr = nlohmann::json::array();
    for (const auto& fd : deps.fileDependencies) {
        fileArr.push_back({ { "file", fd.file }, { "state", fd.state } });
    }
    j["fileDependencies"] = fileArr;

    if (!deps.flagDependencies.empty()) {
        nlohmann::json flagArr = nlohmann::json::array();
        for (const auto& fd : deps.flagDependencies) {
            flagArr.push_back({ { "flag", fd.flag }, { "value", fd.value } });
        }
        j["flagDependencies"] = flagArr;
    }

    if (!deps.nestedDependencies.empty()) {
        nlohmann::json nestedArr = nlohmann::json::array();
        for (const auto& nd : deps.nestedDependencies) {
            nestedArr.push_back(storedDependenciesToJson(nd));
        }
        j["nestedDependencies"] = nestedArr;
    }

    return j;
}

inline StoredDependencies storedDependenciesFromJson(const nlohmann::json& j)
{
    StoredDependencies deps;
    deps.operatorType = j.value("operator", "And");

    if (j.contains("fileDependencies")) {
        for (const auto& fd : j["fileDependencies"]) {
            deps.fileDependencies.push_back({ fd["file"], fd["state"] });
        }
    }

    if (j.contains("flagDependencies")) {
        for (const auto& fd : j["flagDependencies"]) {
            deps.flagDependencies.push_back({ fd["flag"], fd["value"] });
        }
    }

    if (j.contains("nestedDependencies")) {
        for (const auto& nd : j["nestedDependencies"]) {
            deps.nestedDependencies.push_back(storedDependenciesFromJson(nd));
        }
    }

    return deps;
}

struct FomodOption {
    std::string name;
    std::string fileName;
    std::vector<std::string> masters;
    std::string step;
    std::string group;
    SelectionState selectionState = SelectionState::Unknown;
    std::vector<StoredTypePattern> typePatterns;

    FomodOption(std::string n, std::string fn, std::vector<std::string> m, std::string s, std::string g,
        SelectionState state = SelectionState::Unknown, std::vector<StoredTypePattern> tp = {})
        : name(std::move(n))
        , fileName(std::move(fn))
        , masters(std::move(m))
        , step(std::move(s))
        , group(std::move(g))
        , selectionState(state)
        , typePatterns(std::move(tp))
    {
    }
};

class FomodDbEntry {
  public:
    explicit FomodDbEntry(nlohmann::json json)
    {
        modId       = json["modId"];
        displayName = json["displayName"];
        for (auto& option : json["options"]) {
            SelectionState state = SelectionState::Unknown;
            if (option.contains("selectionState")) {
                state = stringToSelectionState(option["selectionState"]);
            }

            std::vector<StoredTypePattern> typePatterns;
            if (option.contains("typePatterns")) {
                for (const auto& tp : option["typePatterns"]) {
                    StoredTypePattern pattern;
                    pattern.type         = tp["type"];
                    pattern.dependencies = storedDependenciesFromJson(tp["dependencies"]);
                    typePatterns.push_back(std::move(pattern));
                }
            }

            FomodOption fomodOption(option["name"], option["fileName"], option["masters"], option["step"],
                option["group"], state, std::move(typePatterns));
            options.push_back(std::move(fomodOption));
        }
    }

    explicit FomodDbEntry(const int modId, std::string displayName, const std::vector<FomodOption>& options)
        : modId(modId)
        , displayName(std::move(displayName))
        , options(options)
    {
    }

    [[nodiscard]] int getModId() const { return modId; }
    [[nodiscard]] std::string getDisplayName() const { return displayName; }
    [[nodiscard]] const std::vector<FomodOption>& getOptions() const { return options; }
    [[nodiscard]] std::vector<FomodOption>& getOptionsMutable() { return options; }

    [[nodiscard]] nlohmann::json toJson() const
    {
        nlohmann::json result;
        result["modId"]       = modId;
        result["displayName"] = displayName;

        nlohmann::json optionsArray = nlohmann::json::array();
        for (const auto& option : options) {
            nlohmann::json optionJson;
            optionJson["name"]           = option.name;
            optionJson["fileName"]       = option.fileName;
            optionJson["masters"]        = option.masters;
            optionJson["step"]           = option.step;
            optionJson["group"]          = option.group;
            optionJson["selectionState"] = selectionStateToString(option.selectionState);

            if (!option.typePatterns.empty()) {
                nlohmann::json patternsArr = nlohmann::json::array();
                for (const auto& tp : option.typePatterns) {
                    patternsArr.push_back({
                        { "type", tp.type },
                        { "dependencies", storedDependenciesToJson(tp.dependencies) },
                    });
                }
                optionJson["typePatterns"] = patternsArr;
            }

            optionsArray.push_back(optionJson);
        }

        result["options"] = optionsArray;
        return result;
    }

    /**
     * Apply user selection states from the FOMOD choices JSON.
     * Matches options by step/group/name key.
     */
    void applySelections(const nlohmann::json& choices)
    {
        if (!choices.contains("steps") || !choices["steps"].is_array()) {
            for (auto& option : options) {
                option.selectionState = SelectionState::Available;
            }
            return;
        }

        struct PluginState {
            bool selected   = false;
            bool deselected = false;
        };
        std::map<std::string, PluginState> stateMap;

        for (const auto& step : choices["steps"]) {
            if (!step.contains("name") || !step.contains("groups"))
                continue;
            const std::string stepName = step["name"];

            for (const auto& group : step["groups"]) {
                if (!group.contains("name"))
                    continue;
                const std::string groupName = group["name"];

                if (group.contains("plugins") && group["plugins"].is_array()) {
                    for (const auto& plugin : group["plugins"]) {
                        const std::string pluginName = plugin;
                        stateMap[stepName + "/" + groupName + "/" + pluginName].selected = true;
                    }
                }
                if (group.contains("deselected") && group["deselected"].is_array()) {
                    for (const auto& plugin : group["deselected"]) {
                        const std::string pluginName = plugin;
                        stateMap[stepName + "/" + groupName + "/" + pluginName].deselected = true;
                    }
                }
            }
        }

        for (auto& option : options) {
            const auto key = option.step + "/" + option.group + "/" + option.name;
            if (auto it = stateMap.find(key); it != stateMap.end()) {
                if (it->second.selected) {
                    option.selectionState = SelectionState::Selected;
                } else if (it->second.deselected) {
                    option.selectionState = SelectionState::Deselected;
                } else {
                    option.selectionState = SelectionState::Available;
                }
            } else {
                option.selectionState = SelectionState::Available;
            }
        }
    }

  private:
    int modId;
    std::string displayName;
    std::vector<FomodOption> options;
};
