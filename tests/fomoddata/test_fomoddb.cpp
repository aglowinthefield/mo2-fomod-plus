#include "FOMODData/FomodDb.h"
#include "FOMODData/FomodDbEntry.h"

#include <fstream>
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>


class FomodDBTest : public ::testing::Test {
protected:
    std::string tempDir;
    std::string dbPath;

    void SetUp() override
    {
        // Create a temporary directory and test DB file
        tempDir = std::filesystem::temp_directory_path().string() + "/fomod_test_" + std::to_string(std::rand());
        std::filesystem::create_directory(tempDir);
        dbPath = tempDir + "/test.db";

        // Create an empty DB file
        std::ofstream file(dbPath);
        file << "[]";
        file.close();
    }

    void TearDown() override
    {
        // Clean up temporary directory
        std::filesystem::remove_all(tempDir);
    }
};

TEST_F(FomodDBTest, ParseFomodDb)
{
    // Parse the JSON object
    FomodDB fomodDb(TEST_DATA_DIR, "test-db.json");

    EXPECT_EQ(fomodDb.getEntries().size(), 2);
    EXPECT_EQ(fomodDb.getEntries()[0]->getOptions()[0].group, "Group One");
    EXPECT_EQ(fomodDb.getEntries()[1]->getOptions()[0].group, "Group Two");
    EXPECT_EQ(fomodDb.getEntries()[1]->getOptions()[0].fileName, "Lux - JK's Arcadia's Cauldron.esp");
}

TEST_F(FomodDBTest, UpsertReplacesExistingEntry)
{
    // Arrange
    FomodDB db(tempDir, "test.db");
    constexpr int modId = 12345;

    // Initial entry with modId 12345
    std::vector<FomodOption> options1 = {
        FomodOption("Option 1", "plugin1.esp", { "master1.esm" }, "Step 1", "Group 1")
    };
    auto entry1 = std::make_unique<FomodDbEntry>(modId, "Original Name", options1);

    // Updated entry with the same modId but different data
    std::vector<FomodOption> options2 = {
        FomodOption("Option 2", "plugin2.esp", { "master2.esm" }, "Step 2", "Group 2")
    };
    auto entry2 = std::make_unique<FomodDbEntry>(modId, "Updated Name", options2);

    // Act
    db.addEntry(std::move(entry1), true);

    // Verify initial state
    ASSERT_EQ(1, db.getEntries().size());
    EXPECT_EQ("Original Name", db.getEntries()[0]->getDisplayName());
    EXPECT_EQ(1, db.getEntries()[0]->getOptions().size());
    EXPECT_EQ("Option 1", db.getEntries()[0]->getOptions()[0].name);

    // Act - add second entry with same modId using upsert
    db.addEntry(std::move(entry2), true);

    // Assert
    EXPECT_EQ(1, db.getEntries().size()); // Should still have only 1 entry
    EXPECT_EQ(modId, db.getEntries()[0]->getModId());
    EXPECT_EQ("Updated Name", db.getEntries()[0]->getDisplayName());
    EXPECT_EQ(1, db.getEntries()[0]->getOptions().size());
    EXPECT_EQ("Option 2", db.getEntries()[0]->getOptions()[0].name);
}

TEST_F(FomodDBTest, AddEntryWithoutUpsertDoesNotReplace)
{
    // Arrange
    FomodDB db(tempDir, "test.db");
    constexpr int modId = 12345;

    // Initial entry with modId 12345
    std::vector<FomodOption> options1 = {
        FomodOption("Option 1", "plugin1.esp", { "master1.esm" }, "Step 1", "Group 1")
    };
    auto entry1 = std::make_unique<FomodDbEntry>(modId, "Original Name", options1);

    // Another entry with the same modId
    std::vector<FomodOption> options2 = {
        FomodOption("Option 2", "plugin2.esp", { "master2.esm" }, "Step 2", "Group 2")
    };
    auto entry2 = std::make_unique<FomodDbEntry>(modId, "New Entry", options2);

    // Act
    db.addEntry(std::move(entry1), true);
    db.addEntry(std::move(entry2), false); // No upsert

    // Assert
    EXPECT_EQ(2, db.getEntries().size()); // Should have 2 entries
}

TEST_F(FomodDBTest, ToJsonSerializesAllEntries)
{
    // Arrange
    FomodDB db(tempDir, "test.db");

    // Create first entry
    std::vector<FomodOption> options1 = {
        FomodOption("Option 1", "plugin1.esp", { "master1.esm" }, "Step 1", "Group 1")
    };
    auto entry1 = std::make_unique<FomodDbEntry>(12345, "First Mod", options1);

    // Create second entry
    std::vector<FomodOption> options2 = {
        FomodOption("Option A", "pluginA.esp", { "masterA.esm" }, "Step A", "Group A"),
        FomodOption("Option B", "pluginB.esp", { "masterB.esm" }, "Step B", "Group B")
    };
    auto entry2 = std::make_unique<FomodDbEntry>(67890, "Second Mod", options2);

    // Add entries to DB
    db.addEntry(std::move(entry1));
    db.addEntry(std::move(entry2));

    // Act - Get JSON
    nlohmann::json result = db.toJson();

    // Assert
    ASSERT_TRUE(result.is_array());
    ASSERT_EQ(2, result.size());

    // Verify first entry
    EXPECT_EQ(12345, result[0]["modId"]);
    EXPECT_EQ("First Mod", result[0]["displayName"]);
    EXPECT_EQ(1, result[0]["options"].size());
    EXPECT_EQ("Option 1", result[0]["options"][0]["name"]);

    // Verify second entry
    EXPECT_EQ(67890, result[1]["modId"]);
    EXPECT_EQ("Second Mod", result[1]["displayName"]);
    EXPECT_EQ(2, result[1]["options"].size());
    EXPECT_EQ("Option A", result[1]["options"][0]["name"]);
    EXPECT_EQ("Option B", result[1]["options"][1]["name"]);
}