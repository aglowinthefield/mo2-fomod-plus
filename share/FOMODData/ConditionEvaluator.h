#pragma once

#include "FomodDBEntry.h"

#include <functional>
#include <string>
#include <unordered_map>

// Callback: given a filename, return its state ("Active", "Inactive", "Missing")
using PluginStateResolver = std::function<std::string(const std::string& fileName)>;

// Creates a caching wrapper around a PluginStateResolver to avoid repeated lookups
inline PluginStateResolver makeCachedResolver(PluginStateResolver inner)
{
    auto cache = std::make_shared<std::unordered_map<std::string, std::string>>();
    return [inner = std::move(inner), cache](const std::string& fileName) -> std::string {
        if (auto it = cache->find(fileName); it != cache->end()) {
            return it->second;
        }
        auto result          = inner(fileName);
        (*cache)[fileName] = result;
        return result;
    };
}

namespace ConditionEvaluator {

inline bool evaluateFileDependency(const StoredFileDependency& dep, const PluginStateResolver& resolver)
{
    return resolver(dep.file) == dep.state;
}

// Evaluates a StoredDependencies block against the resolver.
// Flag dependencies are skipped (not evaluable outside the installer).
// Returns false if there are no evaluable conditions (flag-only blocks).
inline bool evaluateDependencies(const StoredDependencies& deps, const PluginStateResolver& resolver)
{
    std::vector<bool> results;

    for (const auto& fd : deps.fileDependencies) {
        results.push_back(evaluateFileDependency(fd, resolver));
    }

    // Flag dependencies are intentionally skipped — session-local, not evaluable

    for (const auto& nd : deps.nestedDependencies) {
        results.push_back(evaluateDependencies(nd, resolver));
    }

    // If no evaluable conditions exist, this block provides no signal
    if (results.empty()) {
        return false;
    }

    if (deps.operatorType == "Or") {
        return std::ranges::any_of(results, [](bool r) { return r; });
    }
    return std::ranges::all_of(results, [](bool r) { return r; });
}

// First-match semantics: returns the type of the first pattern whose conditions pass.
// Returns empty string if no pattern matches.
// Patterns without file or nested dependencies (static fallbacks, flag-only) are
// skipped since they can't be evaluated against the load order.
inline std::string resolveMatchingType(
    const std::vector<StoredTypePattern>& patterns, const PluginStateResolver& resolver)
{
    for (const auto& pattern : patterns) {
        const bool hasEvaluableConditions = !pattern.dependencies.fileDependencies.empty()
            || !pattern.dependencies.nestedDependencies.empty();

        if (!hasEvaluableConditions) {
            // No file or nested dependencies — skip this pattern.
            // Static type fallbacks (empty deps) and flag-only patterns are not
            // evaluable in the Patch Finder context since they don't reference
            // any external files we can check against the load order.
            continue;
        }

        if (evaluateDependencies(pattern.dependencies, resolver)) {
            return pattern.type;
        }
    }
    return {};
}

} // namespace ConditionEvaluator
