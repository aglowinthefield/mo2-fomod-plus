#include "FOMODData/FomodDbEntry.h"

#include <fstream>
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

TEST(FomodDbEntryTest, ParseFomodDbEntry)
{
    // Path to the test JSON file
    std::string jsonPath = (std::filesystem::path(TEST_DATA_DIR) / "test-entry.json").string();

    // Check if the file exists
    ASSERT_TRUE(std::filesystem::exists(jsonPath)) << "Test JSON file not found: " << jsonPath;

    // Read the JSON file
    std::ifstream file(jsonPath);
    ASSERT_TRUE(file.is_open()) << "Failed to open: " << jsonPath;

    // Parse the JSON content
    nlohmann::json jsonFile = nlohmann::json::parse(file);

    // Parse the JSON object
    FomodDbEntry fomodDbEntry(jsonFile);

    // Verify the modId
    EXPECT_EQ(fomodDbEntry.getModId(), 12345);

    // Verify the displayName
    EXPECT_EQ(fomodDbEntry.getDisplayName(), "Lux (Patch Hub)");

    // Verify the number of options
    EXPECT_EQ(fomodDbEntry.getOptions().size(), 1);

    // Verify the option name
    EXPECT_EQ(fomodDbEntry.getOptions()[0].name, "JK's The Hag's Cure");

    // Verify the option fileName
    EXPECT_EQ(fomodDbEntry.getOptions()[0].fileName, "Lux - JK's The Hag's Cure patch.esp");

    // Verify the number of masters
    EXPECT_EQ(fomodDbEntry.getOptions()[0].masters.size(), 4);

    // Verify each master
    EXPECT_EQ(fomodDbEntry.getOptions()[0].masters[0], "Skyrim.esm");
    EXPECT_EQ(fomodDbEntry.getOptions()[0].masters[1], "JK's The Hag's Cure.esp");
    EXPECT_EQ(fomodDbEntry.getOptions()[0].masters[2], "Lux - Resources.esp");
    EXPECT_EQ(fomodDbEntry.getOptions()[0].masters[3], "Lux.esp");

    // Verify step and group
    EXPECT_EQ(fomodDbEntry.getOptions()[0].step, "Step One");
    EXPECT_EQ(fomodDbEntry.getOptions()[0].group, "Group One");

    // Verify typePatterns
    ASSERT_EQ(fomodDbEntry.getOptions()[0].typePatterns.size(), 1);
    EXPECT_EQ(fomodDbEntry.getOptions()[0].typePatterns[0].type, "Recommended");
    EXPECT_EQ(fomodDbEntry.getOptions()[0].typePatterns[0].dependencies.operatorType, "Or");
    ASSERT_EQ(fomodDbEntry.getOptions()[0].typePatterns[0].dependencies.fileDependencies.size(), 2);
    EXPECT_EQ(fomodDbEntry.getOptions()[0].typePatterns[0].dependencies.fileDependencies[0].file, "TKDodge.esp");
    EXPECT_EQ(fomodDbEntry.getOptions()[0].typePatterns[0].dependencies.fileDependencies[0].state, "Active");
    EXPECT_EQ(
        fomodDbEntry.getOptions()[0].typePatterns[0].dependencies.fileDependencies[1].file, "UltimateCombat.esp");
    EXPECT_EQ(fomodDbEntry.getOptions()[0].typePatterns[0].dependencies.fileDependencies[1].state, "Active");
}

TEST(FomodDbEntryTest, ToJsonSerializesCorrectly)
{
    // Create a FomodOption
    std::vector<std::string> masters
        = { "Skyrim.esm", "JK's The Hag's Cure.esp", "Lux - Resources.esp", "Lux.esp" };
    FomodOption option(
        "JK's The Hag's Cure", "Lux - JK's The Hag's Cure patch.esp", masters, "Step One", "Group One");

    // Create a vector of options
    std::vector<FomodOption> options = { option };

    // Create a FomodDbEntry
    FomodDbEntry entry(12345, "Lux (Patch Hub)", options);

    // Serialize to JSON
    nlohmann::json result = entry.toJson();

    // Verify the JSON structure
    EXPECT_EQ(result["modId"], 12345);
    EXPECT_EQ(result["displayName"], "Lux (Patch Hub)");
    EXPECT_EQ(result["options"].size(), 1);

    // Verify the option JSON
    EXPECT_EQ(result["options"][0]["name"], "JK's The Hag's Cure");
    EXPECT_EQ(result["options"][0]["fileName"], "Lux - JK's The Hag's Cure patch.esp");
    EXPECT_EQ(result["options"][0]["step"], "Step One");
    EXPECT_EQ(result["options"][0]["group"], "Group One");

    // Verify masters array
    EXPECT_EQ(result["options"][0]["masters"].size(), 4);
    EXPECT_EQ(result["options"][0]["masters"][0], "Skyrim.esm");
    EXPECT_EQ(result["options"][0]["masters"][1], "JK's The Hag's Cure.esp");
    EXPECT_EQ(result["options"][0]["masters"][2], "Lux - Resources.esp");
    EXPECT_EQ(result["options"][0]["masters"][3], "Lux.esp");

    // Empty typePatterns should be omitted from JSON
    EXPECT_FALSE(result["options"][0].contains("typePatterns"));
}

TEST(FomodDbEntryTest, BackwardsCompatNoTypePatterns)
{
    // JSON without typePatterns field — should still load fine
    nlohmann::json json = {
        { "modId", 99999 },
        { "displayName", "Old Mod" },
        { "options",
            nlohmann::json::array(
                { { { "name", "Patch" }, { "fileName", "patch.esp" }, { "masters", nlohmann::json::array({ "master.esm" }) },
                    { "step", "S1" }, { "group", "G1" }, { "selectionState", "Available" } } }) },
    };

    FomodDbEntry entry(json);
    EXPECT_EQ(entry.getModId(), 99999);
    EXPECT_EQ(entry.getOptions().size(), 1);
    EXPECT_TRUE(entry.getOptions()[0].typePatterns.empty());
    EXPECT_EQ(entry.getOptions()[0].selectionState, SelectionState::Available);
}

TEST(FomodDbEntryTest, TypePatternsRoundTrip)
{
    // Create an option with typePatterns
    StoredTypePattern pattern;
    pattern.type                 = "Recommended";
    pattern.dependencies.operatorType = "Or";
    pattern.dependencies.fileDependencies.push_back({ "AOS.esp", "Active" });
    pattern.dependencies.fileDependencies.push_back({ "ELFX.esp", "Active" });
    pattern.dependencies.flagDependencies.push_back({ "MyFlag", "On" });

    FomodOption option("AOS Patch", "AOS_Patch.esp", { "master.esm" }, "Patches", "Audio",
        SelectionState::Available, { pattern });
    FomodDbEntry entry(1000, "Test Mod", { option });

    // Serialize
    nlohmann::json json = entry.toJson();

    // Verify serialized structure
    ASSERT_TRUE(json["options"][0].contains("typePatterns"));
    ASSERT_EQ(json["options"][0]["typePatterns"].size(), 1);
    EXPECT_EQ(json["options"][0]["typePatterns"][0]["type"], "Recommended");
    EXPECT_EQ(json["options"][0]["typePatterns"][0]["dependencies"]["operator"], "Or");
    EXPECT_EQ(json["options"][0]["typePatterns"][0]["dependencies"]["fileDependencies"].size(), 2);
    EXPECT_EQ(json["options"][0]["typePatterns"][0]["dependencies"]["flagDependencies"].size(), 1);

    // Deserialize back
    FomodDbEntry roundTripped(json);
    ASSERT_EQ(roundTripped.getOptions().size(), 1);
    ASSERT_EQ(roundTripped.getOptions()[0].typePatterns.size(), 1);

    const auto& tp = roundTripped.getOptions()[0].typePatterns[0];
    EXPECT_EQ(tp.type, "Recommended");
    EXPECT_EQ(tp.dependencies.operatorType, "Or");
    ASSERT_EQ(tp.dependencies.fileDependencies.size(), 2);
    EXPECT_EQ(tp.dependencies.fileDependencies[0].file, "AOS.esp");
    EXPECT_EQ(tp.dependencies.fileDependencies[0].state, "Active");
    EXPECT_EQ(tp.dependencies.fileDependencies[1].file, "ELFX.esp");
    ASSERT_EQ(tp.dependencies.flagDependencies.size(), 1);
    EXPECT_EQ(tp.dependencies.flagDependencies[0].flag, "MyFlag");
    EXPECT_EQ(tp.dependencies.flagDependencies[0].value, "On");
}

TEST(FomodDbEntryTest, NestedDependenciesRoundTrip)
{
    // Create nested dependencies: AND(file1, OR(file2, file3))
    StoredDependencies inner;
    inner.operatorType = "Or";
    inner.fileDependencies.push_back({ "ELFX.esp", "Active" });
    inner.fileDependencies.push_back({ "SMIM.esp", "Active" });

    StoredTypePattern pattern;
    pattern.type                 = "Required";
    pattern.dependencies.operatorType = "And";
    pattern.dependencies.fileDependencies.push_back({ "AOS.esp", "Active" });
    pattern.dependencies.nestedDependencies.push_back(inner);

    FomodOption option("Nested", "nested.esp", {}, "S", "G", SelectionState::Unknown, { pattern });
    FomodDbEntry entry(2000, "Nested Mod", { option });

    // Round-trip
    nlohmann::json json = entry.toJson();
    FomodDbEntry roundTripped(json);

    const auto& tp = roundTripped.getOptions()[0].typePatterns[0];
    EXPECT_EQ(tp.dependencies.operatorType, "And");
    ASSERT_EQ(tp.dependencies.fileDependencies.size(), 1);
    ASSERT_EQ(tp.dependencies.nestedDependencies.size(), 1);
    EXPECT_EQ(tp.dependencies.nestedDependencies[0].operatorType, "Or");
    ASSERT_EQ(tp.dependencies.nestedDependencies[0].fileDependencies.size(), 2);
    EXPECT_EQ(tp.dependencies.nestedDependencies[0].fileDependencies[0].file, "ELFX.esp");
    EXPECT_EQ(tp.dependencies.nestedDependencies[0].fileDependencies[1].file, "SMIM.esp");
}
