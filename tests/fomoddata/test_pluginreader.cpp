#include "FOMODData/PluginReader.h"
#include <gtest/gtest.h>
#include <algorithm>
#include <filesystem>

TEST(PluginReaderTest, ReadMasters) {
    // Path to the test ESP
    std::string testEspPath = (std::filesystem::path(TEST_DATA_DIR) / "Lux - JK's The Hag's Cure patch.esp").string();

    // Verify the file exists
    ASSERT_TRUE(std::filesystem::exists(testEspPath))
        << "Test file not found: " << testEspPath;

    // Verify it's a valid plugin
    ASSERT_TRUE(PluginReader::isValidPlugin(testEspPath))
        << "Test file is not a valid plugin: " << testEspPath;

    // Read masters
    std::vector<std::string> masters = PluginReader::readMasters(testEspPath);

    // Expected masters
    std::vector<std::string> expectedMasters = {
        "Skyrim.esm",
        "JK's The Hag's Cure.esp",
        "Lux - Resources.esp",
        "Lux.esp"
    };

    // Verify we got the right number of masters
    ASSERT_EQ(masters.size(), expectedMasters.size())
        << "Incorrect number of masters. Expected " << expectedMasters.size()
        << " but got " << masters.size();

    // Verify each master is present
    for (const auto& expectedMaster : expectedMasters) {
        EXPECT_TRUE(std::find(masters.begin(), masters.end(), expectedMaster) != masters.end())
            << "Expected master not found: " << expectedMaster;
    }

    // Verify masters are in the correct order
    for (size_t i = 0; i < expectedMasters.size(); i++) {
        EXPECT_EQ(masters[i], expectedMasters[i])
            << "Master at position " << i << " doesn't match expected value. "
            << "Expected: " << expectedMasters[i] << ", Got: " << masters[i];
    }
}

TEST(PluginReaderTest, InvalidPlugin) {
    // Test with non-existent file
    EXPECT_FALSE(PluginReader::isValidPlugin("non_existent_file.esp"));

    // Test with invalid plugin file (if you have a non-plugin file to test with)
    // EXPECT_FALSE(PluginReader::isValidPlugin("some_text_file.txt"));

    // Reading masters from invalid file should return empty vector
    EXPECT_TRUE(PluginReader::readMasters("non_existent_file.esp").empty());
}