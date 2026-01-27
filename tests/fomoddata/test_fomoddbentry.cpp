#include "FOMODData/FomodDbEntry.h"

#include <fstream>
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

TEST(FomodDbEntryTest, ParseFomodDbEntry) {
    // Path to the test JSON file
    std::string jsonPath = (std::filesystem::path(TEST_DATA_DIR) / "test-entry.json").string();

    // Check if the file exists
    ASSERT_TRUE(std::filesystem::exists(jsonPath))
        << "Test JSON file not found: " << jsonPath;

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
}

TEST(FomodDbEntryTest, ToJsonSerializesCorrectly) {
    // Create a FomodOption
    std::vector<std::string> masters = {
        "Skyrim.esm",
        "JK's The Hag's Cure.esp",
        "Lux - Resources.esp",
        "Lux.esp"
    };
    FomodOption option("JK's The Hag's Cure", "Lux - JK's The Hag's Cure patch.esp",
                        masters, "Step One", "Group One");

    // Create a vector of options
    std::vector<FomodOption> options = {option};

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
}
