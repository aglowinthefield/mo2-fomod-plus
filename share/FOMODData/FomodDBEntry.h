﻿#pragma once

#include <string>
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
            "group": "Group One"
        }
    ]
}
*/

struct FomodOption {
  std::string name;
  std::string fileName;
  std::vector<std::string> masters;
  std::string step;
  std::string group;
};

class FomodDbEntry {
public:
  explicit FomodDbEntry(nlohmann::json json) {
    modId = json["modId"];
    displayName = json["displayName"];
    for (auto &option: json["options"]) {
      // create an option from this object
      FomodOption fomodOption(
        option["name"],
        option["fileName"],
        option["masters"],
        option["step"],
        option["group"]
      );
      options.push_back(fomodOption);
    }
  }

  explicit FomodDbEntry(const int modId, const std::string &displayName, const std::vector<FomodOption> &options)
    : modId(modId), displayName(displayName), options(options) {
  }

  [[nodiscard]] int getModId() const { return modId; }
  [[nodiscard]] std::string getDisplayName() const { return displayName; }
  std::vector<FomodOption> getOptions() { return options; }

private:
  int modId;
  std::string displayName;
  std::vector<FomodOption> options;
};
