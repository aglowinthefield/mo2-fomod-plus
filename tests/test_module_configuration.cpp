#include <gtest/gtest.h>
#include "../src/xml/ModuleConfiguration.h"
#include <filesystem>

class ModuleConfigurationTest : public ::testing::Test {
protected:
    ModuleConfiguration moduleConfig;

    void SetUp() override {
        std::string filePath = (std::filesystem::path(__FILE__).parent_path() / "test_fomod_moduleconf.xml").string();
        moduleConfig.deserialize(filePath);
    }
};

TEST_F(ModuleConfigurationTest, ModuleName) {
    EXPECT_EQ(moduleConfig.moduleName, "xavbio's meshes for 3BA");
}

TEST_F(ModuleConfigurationTest, InstallStepsCount) {
    ASSERT_EQ(moduleConfig.installSteps.installSteps.size(), 3);
}

TEST_F(ModuleConfigurationTest, FirstInstallStepName) {
    EXPECT_EQ(moduleConfig.installSteps.installSteps[0].name, "xavbio's mods installed");
}

TEST_F(ModuleConfigurationTest, FirstInstallStepGroupCount) {
    ASSERT_EQ(moduleConfig.installSteps.installSteps[0].optionalFileGroups.groups.size(), 1);
}

TEST_F(ModuleConfigurationTest, FirstGroupName) {
    EXPECT_EQ(moduleConfig.installSteps.installSteps[0].optionalFileGroups.groups[0].name, "Mods");
}

TEST_F(ModuleConfigurationTest, FirstGroupPluginCount) {
    ASSERT_EQ(moduleConfig.installSteps.installSteps[0].optionalFileGroups.groups[0].plugins.plugins.size(), 2);
}

TEST_F(ModuleConfigurationTest, FirstPluginName) {
    EXPECT_EQ(moduleConfig.installSteps.installSteps[0].optionalFileGroups.groups[0].plugins.plugins[0].name,
              "Ancient Nord Armors");
}

TEST_F(ModuleConfigurationTest, FirstPluginImagePath) {
    EXPECT_EQ(moduleConfig.installSteps.installSteps[0].optionalFileGroups.groups[0].plugins.plugins[0].image.path,
              "fomod\\91136-1683833629-107808357_png.jpg");
}

TEST_F(ModuleConfigurationTest, SecondPluginName) {
    EXPECT_EQ(moduleConfig.installSteps.installSteps[0].optionalFileGroups.groups[0].plugins.plugins[1].name,
              "Imperial Armors");
}

TEST_F(ModuleConfigurationTest, SecondPluginImagePath) {
    EXPECT_EQ(moduleConfig.installSteps.installSteps[0].optionalFileGroups.groups[0].plugins.plugins[1].image.path,
              "fomod\\86097-1677724584-1488206295_png.jpg");
}
