﻿#ifndef FLAGMAP_H
#define FLAGMAP_H
#include "ViewModels.h"
#include "stringutil.h"

#include <string>
#include <unordered_map>

using Flag = std::pair<std::string, std::string>;
using FlagList = std::vector<Flag>;

class FlagMap {
public:
    [[nodiscard]] FlagList getFlagsByKey(const std::string& key) const
    {
        FlagList result;
        for (const auto& [plugin, flags] : flags) {
            for (const auto& flag : flags) {
                if (toLower(flag.first) == toLower(key)) {
                    result.emplace_back(flag);
                }
            }
        }
        return result;
    }

    void setFlagsForPlugin(PluginRef plugin)
    {
        // Don't clutter the map with empty key-vals
        if (plugin->getConditionFlags().size() == 0) {
            return;
        }
        unsetFlagsForPlugin(plugin);

        FlagList flagList;;
        for (auto conditionFlag : plugin->getConditionFlags()) {
            flagList.emplace_back(toLower(conditionFlag.name), conditionFlag.value);
        }
        flags[plugin] = flagList;
    }

    void unsetFlagsForPlugin(PluginRef plugin)
    {
        if (flags.contains(plugin)) {
            flags.erase(plugin);
        }
    }

    std::string toString()
    {
        auto result = std::string();
        result += "FlagMap:\n";

        for (const auto& [plugin, flags] : flags) {
            result += plugin->getName() + " [";
            for (const auto& [fst, snd] : flags) {
                result += fst + ": " + snd + ", ";
            }
            result.erase(result.size() - 2);
            result += "]\n";
        }
        return result;
    }

    void clearAll()
    {
        flags.clear();
    }

    [[nodiscard]] size_t getFlagCount() const
    {
        return flags.size();
    }


private:
    std::unordered_map<std::shared_ptr<PluginViewModel>, FlagList> flags;
};
#endif //FLAGMAP_H