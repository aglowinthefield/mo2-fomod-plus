#pragma once

#include "ViewModels.h"
#include "stringutil.h"

#include <ranges>
#include <string>
#include <unordered_map>

using Flag = std::pair<std::string, std::string>;
using FlagList = std::vector<Flag>;

class FlagMap {
public:

    [[nodiscard]] std::vector<std::shared_ptr<PluginViewModel>> getPluginsSettingFlag(const std::string& key, const std::string& value) const
    {
        std::vector<std::shared_ptr<PluginViewModel>> result;
        for (const auto& [plugin, theseFlags] : flags) {
            for (const auto& [fst, snd] : theseFlags) {
                if (toLower(fst) == toLower(key) && snd == value) {
                    result.emplace_back(plugin);
                }
            }
        }
        return result;
    }

    /**
     *
     * @param key The flag key
     * @return A list of flags currently set in this map with the given key. The list is ordered by step descending, then plugin ascending.
     * So if steps 1, 2, and 3 set flag X in their first two plugins, it'll be ordered [3:1, 3:2, 2:1, 2:2, 1:2, 1:1]
     */
    [[nodiscard]] FlagList getFlagsByKey(const std::string& key) const
    {
        FlagList result;
        std::vector<std::pair<int, std::shared_ptr<PluginViewModel>>> orderedPlugins;

        // Collect all plugins with their stepIndex and ownIndex
        for (const auto& plugin : flags | std::views::keys) {
            orderedPlugins.emplace_back(plugin->getStepIndex(), plugin);
        }

        // Sort plugins by stepIndex and ownIndex, stepIndex descending and ownIndex ascending
        // TODO: Might need group sorting too. How can I just do the natural order??
        std::ranges::sort(orderedPlugins, [](const auto& a, const auto& b) {
            return a.first > b.first || (a.first == b.first && a.second->getOwnIndex() < b.second->getOwnIndex());
        });

        // Collect flags in the sorted order
        for (const auto& plugin : orderedPlugins | std::views::values) {
            for (const auto& flag : flags.at(plugin)) {
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
        if (plugin->getConditionFlags().empty()) {
            return;
        }
        unsetFlagsForPlugin(plugin);

        FlagList flagList;
        for (const auto& conditionFlag : plugin->getConditionFlags()) {
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

        for (const auto& [plugin, theseFlags] : flags) {
            result += plugin->getName() + " [";
            for (const auto& [fst, snd] : theseFlags) {
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
