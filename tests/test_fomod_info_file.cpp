#include <gtest/gtest.h>

#include <filesystem>

#include "../installer/xml/FomodInfoFile.h"

TEST(FomodInfoFileTest, DeserializeValidFile)
{
    FomodInfoFile fomodInfo;
    std::string filePath = (std::filesystem::path(__FILE__).parent_path() / "test_fomod.xml").string();
    bool result          = fomodInfo.deserialize(QString::fromStdString(filePath));
    EXPECT_TRUE(result);
    EXPECT_EQ(fomodInfo.getName(), "Ancient Nord Armors and Weapons Retexture SE");
    EXPECT_EQ(fomodInfo.getAuthor(), "xavbio");
    EXPECT_EQ(fomodInfo.getVersion(), "1.0");
    EXPECT_EQ(fomodInfo.getWebsite(), "https://www.nexusmods.com/skyrimspecialedition/mods/91136");
    EXPECT_FALSE(fomodInfo.getDescription().empty());
    EXPECT_EQ(fomodInfo.getGroups().size(), 1);
    EXPECT_EQ(fomodInfo.getGroups()[0], "models and Textures");
}

int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}