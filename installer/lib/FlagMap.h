#ifndef FLAGMAP_H
#define FLAGMAP_H
#include "ViewModels.h"
#include "stringutil.h"

#include <ranges>
#include <string>
#include <unordered_map>

using Flag = std::pair<std::string, std::string>;
using FlagList = std::vector<Flag>;

class FlagMap {
public:

    // TODO: This needs to go backward from step, forward within step.
    [[nodiscard]] FlagList getFlagsByKey(const std::string& key) const
    {
        FlagList result;
        std::vector<std::pair<int, std::shared_ptr<PluginViewModel>>> orderedPlugins;

        // Collect all plugins with their stepIndex and ownIndex
        for (const auto& [plugin, flags] : flags) {
            orderedPlugins.emplace_back(plugin->getStepIndex(), plugin);
        }

        // Sort plugins by stepIndex and ownIndex, stepIndex descending and ownIndex ascending
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