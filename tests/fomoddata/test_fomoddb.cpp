#include "FOMODData/FomodDb.h"
#include "FOMODData/FomodDbEntry.h"

#include <fstream>
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

TEST(FomodDbTest, ParseFomodDb) {
    // Parse the JSON object
    FomodDB fomodDb(TEST_DATA_DIR, "test-db.json");

    EXPECT_EQ(fomodDb.getEntries().size(), 2);
    EXPECT_EQ(fomodDb.getEntries()[0]->getOptions()[0].group, "Group One");
    EXPECT_EQ(fomodDb.getEntries()[1]->getOptions()[0].group, "Group Two");
    EXPECT_EQ(fomodDb.getEntries()[1]->getOptions()[0].fileName, "Lux - JK's Arcadia's Cauldron.esp");
}
