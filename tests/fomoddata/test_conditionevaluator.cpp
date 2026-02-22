#include "FOMODData/ConditionEvaluator.h"

#include <gtest/gtest.h>

// Helper: create a mock resolver from a map
PluginStateResolver makeResolver(const std::unordered_map<std::string, std::string>& states)
{
    return [states](const std::string& fileName) -> std::string {
        if (auto it = states.find(fileName); it != states.end()) {
            return it->second;
        }
        return "Missing";
    };
}

// --- evaluateFileDependency ---

TEST(ConditionEvaluatorTest, FileDependencyActive)
{
    StoredFileDependency dep { "AOS.esp", "Active" };
    auto resolver = makeResolver({ { "AOS.esp", "Active" } });
    EXPECT_TRUE(ConditionEvaluator::evaluateFileDependency(dep, resolver));
}

TEST(ConditionEvaluatorTest, FileDependencyInactive)
{
    StoredFileDependency dep { "AOS.esp", "Active" };
    auto resolver = makeResolver({ { "AOS.esp", "Inactive" } });
    EXPECT_FALSE(ConditionEvaluator::evaluateFileDependency(dep, resolver));
}

TEST(ConditionEvaluatorTest, FileDependencyMissing)
{
    StoredFileDependency dep { "AOS.esp", "Active" };
    auto resolver = makeResolver({}); // Not in map → "Missing"
    EXPECT_FALSE(ConditionEvaluator::evaluateFileDependency(dep, resolver));
}

TEST(ConditionEvaluatorTest, FileDependencyCheckMissing)
{
    StoredFileDependency dep { "AOS.esp", "Missing" };
    auto resolver = makeResolver({}); // Not in map → "Missing"
    EXPECT_TRUE(ConditionEvaluator::evaluateFileDependency(dep, resolver));
}

TEST(ConditionEvaluatorTest, FileDependencyCheckInactive)
{
    StoredFileDependency dep { "AOS.esp", "Inactive" };
    auto resolver = makeResolver({ { "AOS.esp", "Inactive" } });
    EXPECT_TRUE(ConditionEvaluator::evaluateFileDependency(dep, resolver));
}

// --- evaluateDependencies AND/OR ---

TEST(ConditionEvaluatorTest, AndOperatorAllTrue)
{
    StoredDependencies deps;
    deps.operatorType = "And";
    deps.fileDependencies.push_back({ "A.esp", "Active" });
    deps.fileDependencies.push_back({ "B.esp", "Active" });

    auto resolver = makeResolver({ { "A.esp", "Active" }, { "B.esp", "Active" } });
    EXPECT_TRUE(ConditionEvaluator::evaluateDependencies(deps, resolver));
}

TEST(ConditionEvaluatorTest, AndOperatorOneFalse)
{
    StoredDependencies deps;
    deps.operatorType = "And";
    deps.fileDependencies.push_back({ "A.esp", "Active" });
    deps.fileDependencies.push_back({ "B.esp", "Active" });

    auto resolver = makeResolver({ { "A.esp", "Active" }, { "B.esp", "Missing" } });
    EXPECT_FALSE(ConditionEvaluator::evaluateDependencies(deps, resolver));
}

TEST(ConditionEvaluatorTest, OrOperatorOneTrue)
{
    StoredDependencies deps;
    deps.operatorType = "Or";
    deps.fileDependencies.push_back({ "A.esp", "Active" });
    deps.fileDependencies.push_back({ "B.esp", "Active" });

    auto resolver = makeResolver({ { "A.esp", "Missing" }, { "B.esp", "Active" } });
    EXPECT_TRUE(ConditionEvaluator::evaluateDependencies(deps, resolver));
}

TEST(ConditionEvaluatorTest, OrOperatorNoneTrue)
{
    StoredDependencies deps;
    deps.operatorType = "Or";
    deps.fileDependencies.push_back({ "A.esp", "Active" });
    deps.fileDependencies.push_back({ "B.esp", "Active" });

    auto resolver = makeResolver({}); // Both missing
    EXPECT_FALSE(ConditionEvaluator::evaluateDependencies(deps, resolver));
}

// --- evaluateDependencies with nested ---

TEST(ConditionEvaluatorTest, NestedDependencies)
{
    // AND(A=Active, OR(B=Active, C=Active))
    StoredDependencies inner;
    inner.operatorType = "Or";
    inner.fileDependencies.push_back({ "B.esp", "Active" });
    inner.fileDependencies.push_back({ "C.esp", "Active" });

    StoredDependencies deps;
    deps.operatorType = "And";
    deps.fileDependencies.push_back({ "A.esp", "Active" });
    deps.nestedDependencies.push_back(inner);

    // A active, B missing, C active → AND(true, OR(false, true)) → true
    auto resolver = makeResolver({ { "A.esp", "Active" }, { "C.esp", "Active" } });
    EXPECT_TRUE(ConditionEvaluator::evaluateDependencies(deps, resolver));

    // A active, B missing, C missing → AND(true, OR(false, false)) → false
    auto resolver2 = makeResolver({ { "A.esp", "Active" } });
    EXPECT_FALSE(ConditionEvaluator::evaluateDependencies(deps, resolver2));
}

// --- Flag-only dependencies ---

TEST(ConditionEvaluatorTest, FlagOnlyReturnsFalse)
{
    StoredDependencies deps;
    deps.operatorType = "And";
    deps.flagDependencies.push_back({ "MyFlag", "On" });
    // No file dependencies → no evaluable conditions → false

    auto resolver = makeResolver({});
    EXPECT_FALSE(ConditionEvaluator::evaluateDependencies(deps, resolver));
}

TEST(ConditionEvaluatorTest, MixedFileAndFlagSkipsFlags)
{
    // AND operator with file dep (true) and flag dep (skipped)
    // Only file dep contributes → result is true
    StoredDependencies deps;
    deps.operatorType = "And";
    deps.fileDependencies.push_back({ "A.esp", "Active" });
    deps.flagDependencies.push_back({ "MyFlag", "On" });

    auto resolver = makeResolver({ { "A.esp", "Active" } });
    EXPECT_TRUE(ConditionEvaluator::evaluateDependencies(deps, resolver));
}

// --- Empty dependencies (static fallback) ---

TEST(ConditionEvaluatorTest, EmptyDependenciesReturnsFalse)
{
    // Empty deps with no flags → evaluateDependencies returns false (no signal)
    StoredDependencies deps;
    deps.operatorType = "And";

    auto resolver = makeResolver({});
    EXPECT_FALSE(ConditionEvaluator::evaluateDependencies(deps, resolver));
}

// --- resolveMatchingType ---

TEST(ConditionEvaluatorTest, ResolveMatchingTypeFirstMatch)
{
    StoredTypePattern p1;
    p1.type                 = "Recommended";
    p1.dependencies.operatorType = "And";
    p1.dependencies.fileDependencies.push_back({ "AOS.esp", "Active" });

    auto resolver = makeResolver({ { "AOS.esp", "Active" } });
    auto result   = ConditionEvaluator::resolveMatchingType({ p1 }, resolver);
    EXPECT_EQ(result, "Recommended");
}

TEST(ConditionEvaluatorTest, ResolveMatchingTypeNoMatch)
{
    StoredTypePattern p1;
    p1.type                 = "Recommended";
    p1.dependencies.operatorType = "And";
    p1.dependencies.fileDependencies.push_back({ "AOS.esp", "Active" });

    auto resolver = makeResolver({}); // AOS not present
    auto result   = ConditionEvaluator::resolveMatchingType({ p1 }, resolver);
    EXPECT_EQ(result, "");
}

TEST(ConditionEvaluatorTest, ResolveMatchingTypeFirstMatchSemantics)
{
    // Pattern 1: NotUsable if AOS active
    StoredTypePattern p1;
    p1.type                 = "NotUsable";
    p1.dependencies.operatorType = "And";
    p1.dependencies.fileDependencies.push_back({ "AOS.esp", "Active" });

    // Pattern 2: Recommended if ELFX active
    StoredTypePattern p2;
    p2.type                 = "Recommended";
    p2.dependencies.operatorType = "And";
    p2.dependencies.fileDependencies.push_back({ "ELFX.esp", "Active" });

    // Both AOS and ELFX active → first match wins → NotUsable
    auto resolver = makeResolver({ { "AOS.esp", "Active" }, { "ELFX.esp", "Active" } });
    auto result   = ConditionEvaluator::resolveMatchingType({ p1, p2 }, resolver);
    EXPECT_EQ(result, "NotUsable");
}

TEST(ConditionEvaluatorTest, ResolveMatchingTypeSkipsNonMatchingFirst)
{
    // Pattern 1: NotUsable if AOS active (won't match)
    StoredTypePattern p1;
    p1.type                 = "NotUsable";
    p1.dependencies.operatorType = "And";
    p1.dependencies.fileDependencies.push_back({ "AOS.esp", "Active" });

    // Pattern 2: Recommended if ELFX active (will match)
    StoredTypePattern p2;
    p2.type                 = "Recommended";
    p2.dependencies.operatorType = "And";
    p2.dependencies.fileDependencies.push_back({ "ELFX.esp", "Active" });

    // Only ELFX active → first match is p2 → Recommended
    auto resolver = makeResolver({ { "ELFX.esp", "Active" } });
    auto result   = ConditionEvaluator::resolveMatchingType({ p1, p2 }, resolver);
    EXPECT_EQ(result, "Recommended");
}

TEST(ConditionEvaluatorTest, ResolveMatchingTypeEmptyDepsStaticFallback)
{
    // Static fallback: empty dependencies (no flags) → always matches
    StoredTypePattern p1;
    p1.type                 = "Recommended";
    p1.dependencies.operatorType = "And";
    // No file deps, no flag deps, no nested deps

    auto resolver = makeResolver({});
    auto result   = ConditionEvaluator::resolveMatchingType({ p1 }, resolver);
    EXPECT_EQ(result, "Recommended");
}

TEST(ConditionEvaluatorTest, ResolveMatchingTypeFlagOnlySkipped)
{
    // Flag-only pattern → skipped (non-evaluable)
    StoredTypePattern p1;
    p1.type                 = "Recommended";
    p1.dependencies.operatorType = "And";
    p1.dependencies.flagDependencies.push_back({ "SomeFlag", "On" });

    auto resolver = makeResolver({});
    auto result   = ConditionEvaluator::resolveMatchingType({ p1 }, resolver);
    EXPECT_EQ(result, "");
}

TEST(ConditionEvaluatorTest, ResolveMatchingTypeEmptyPatterns)
{
    auto resolver = makeResolver({});
    auto result   = ConditionEvaluator::resolveMatchingType({}, resolver);
    EXPECT_EQ(result, "");
}

// --- makeCachedResolver ---

TEST(ConditionEvaluatorTest, CachedResolverCachesResults)
{
    int callCount = 0;
    auto inner    = [&callCount](const std::string& fileName) -> std::string {
        callCount++;
        return "Active";
    };

    auto cached = makeCachedResolver(inner);
    EXPECT_EQ(cached("test.esp"), "Active");
    EXPECT_EQ(cached("test.esp"), "Active");
    EXPECT_EQ(cached("test.esp"), "Active");
    EXPECT_EQ(callCount, 1); // Only called once despite 3 lookups
}
