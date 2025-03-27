#include "FOMODData/FomodDb.h"
#include "FOMODData/FomodDbEntry.h"

#include <fstream>
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

TEST(FomodDbTest, ParseFomodDb) {
    // Path to the test JSON file
    std::string jsonPath = (std::filesystem::path(TEST_DATA_DIR) / "test-db.json").string();

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
}
