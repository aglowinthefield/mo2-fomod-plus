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
// Empty dependencies (no file deps, no nested deps) always evaluate to true —
// this allows static type fallbacks (e.g., unconditionally Recommended) to match.
inline std::string resolveMatchingType(
    const std::vector<StoredTypePattern>& patterns, const PluginStateResolver& resolver)
{
    for (const auto& pattern : patterns) {
        const bool hasEvaluableConditions = !pattern.dependencies.fileDependencies.empty()
            || !pattern.dependencies.nestedDependencies.empty();

        if (!hasEvaluableConditions) {
            // No evaluable conditions — but also check if there are flag-only conditions.
            // If there are ONLY flag deps, skip this pattern (non-evaluable).
            // If there are NO conditions at all, this is a static fallback — always matches.
            if (!pattern.dependencies.flagDependencies.empty()) {
                continue; // Flag-only pattern, can't evaluate
            }
            return pattern.type; // Empty dependencies = always true (static type fallback)
        }

        if (evaluateDependencies(pattern.dependencies, resolver)) {
            return pattern.type;
        }
    }
    return {};
}

} // namespace ConditionEvaluator
