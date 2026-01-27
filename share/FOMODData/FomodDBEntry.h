#pragma once

#include <string>
#include <utility>
#include <vector>
#include <nlohmann/json.hpp>

/*
The following JSON will be part of an array of similar objects in the root level "JSON DB" for FOMOD Plus.
It contains information to resolve the identity of a given mod (names can change), and then the options
in the FOMOD with their respective masters.
{
    modId: 12345,
    displayName: "Lux (Patch Hub)",
    options: [
        {
            "name" "JK's The Hag's Cure",
            "fileName": "Lux - JK's The Hag's Cure patch.esp",
            "masters": [
                "Skyrim.esm",
                "JK's The Hag's Cure.esp",
                "Lux - Resources.esp",
                "Lux.esp"
            ],
            "step": "Page One",
            "group": "Group One",
            "selectionState": "Available"
        }
    ]
}
*/

enum class SelectionState {
  Unknown,     // Not yet matched to choices
  Selected,    // User selected this option
  Deselected,  // User manually deselected
  Available    // Present but user didn't interact (or choices not recorded)
};

inline std::string selectionStateToString(SelectionState state) {
  switch (state) {
    case SelectionState::Unknown: return "Unknown";
    case SelectionState::Selected: return "Selected";
    case SelectionState::Deselected: return "Deselected";
    case SelectionState::Available: return "Available";
    default: return "Unknown";
  }
}

inline SelectionState stringToSelectionState(const std::string& str) {
  if (str == "Selected") return SelectionState::Selected;
  if (str == "Deselected") return SelectionState::Deselected;
  if (str == "Available") return SelectionState::Available;
  return SelectionState::Unknown;
}

struct FomodOption {
  std::string name;
  std::string fileName;
  std::vector<std::string> masters;
  std::string step;
  std::string group;
  SelectionState selectionState = SelectionState::Unknown;

  FomodOption(std::string n, std::string fn, std::vector<std::string> m, std::string s, std::string g,
              SelectionState state = SelectionState::Unknown)
  : name(std::move(n)), fileName(std::move(fn)), masters(std::move(m)), step(std::move(s)), group(std::move(g)),
    selectionState(state) {}
};

class FomodDbEntry {
public:
  explicit FomodDbEntry(nlohmann::json json) {
    modId = json["modId"];
    displayName = json["displayName"];
    for (auto &option: json["options"]) {
      // create an option from this object
      SelectionState state = SelectionState::Unknown;
      if (option.contains("selectionState")) {
        state = stringToSelectionState(option["selectionState"]);
      }
      FomodOption fomodOption(
        option["name"],
        option["fileName"],
        option["masters"],
        option["step"],
        option["group"],
        state
      );
      options.push_back(fomodOption);
    }
  }

  explicit FomodDbEntry(const int modId, std::string displayName, const std::vector<FomodOption> &options)
    : modId(modId), displayName(std::move(displayName)), options(options) {
  }


  [[nodiscard]] int getModId() const { return modId; }
  [[nodiscard]] std::string getDisplayName() const { return displayName; }
  [[nodiscard]] const std::vector<FomodOption>& getOptions() const { return options; }
  [[nodiscard]] std::vector<FomodOption>& getOptionsMutable() { return options; }

  [[nodiscard]] nlohmann::json toJson() const {
    nlohmann::json result;
    result["modId"] = modId;
    result["displayName"] = displayName;

    nlohmann::json optionsArray = nlohmann::json::array();
    for (const auto &[name, fileName, masters, step, group, selectionState]: options) {
      nlohmann::json optionJson;
      optionJson["name"] = name;
      optionJson["fileName"] = fileName;
      optionJson["masters"] = masters;
      optionJson["step"] = step;
      optionJson["group"] = group;
      optionJson["selectionState"] = selectionStateToString(selectionState);
      optionsArray.push_back(optionJson);
    }

    result["options"] = optionsArray;
    return result;
  }

private:
  int modId;
  std::string displayName;
  std::vector<FomodOption> options;
};
