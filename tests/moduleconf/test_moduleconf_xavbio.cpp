#include <gtest/gtest.h>
#include "../../installer/xml/ModuleConfiguration.h"
#include <filesystem>

class ModuleConfigurationTest_Xavbio : public ::testing::Test {
protected:
    ModuleConfiguration moduleConfig;

    void SetUp() override
    {
        std::string filePath = (std::filesystem::path(__FILE__).parent_path() / "test_moduleconf_xavbio.xml").string();
        moduleConfig.deserialize(filePath);
    }
};

TEST_F(ModuleConfigurationTest_Xavbio, ModuleName)
{
    EXPECT_EQ(moduleConfig.moduleName, "xavbio's meshes for 3BA");
}

TEST_F(ModuleConfigurationTest_Xavbio, InstallStepsCount)
{
    ASSERT_EQ(moduleConfig.installSteps.installSteps.size(), 3);
}

TEST_F(ModuleConfigurationTest_Xavbio, FirstInstallStepName)
{
    EXPECT_EQ(moduleConfig.installSteps.installSteps[0].name, "xavbio's mods installed");
}

TEST_F(ModuleConfigurationTest_Xavbio, FirstInstallStepGroupCount)
{
    ASSERT_EQ(moduleConfig.installSteps.installSteps[0].optionalFileGroups.groups.size(), 1);
}

TEST_F(ModuleConfigurationTest_Xavbio, FirstGroupName)
{
    EXPECT_EQ(moduleConfig.installSteps.installSteps[0].optionalFileGroups.groups[0].name, "Mods");
}

TEST_F(ModuleConfigurationTest_Xavbio, FirstGroupPluginCount)
{
    ASSERT_EQ(moduleConfig.installSteps.installSteps[0].optionalFileGroups.groups[0].plugins.plugins.size(), 2);
}

TEST_F(ModuleConfigurationTest_Xavbio, FirstPluginName)
{
    EXPECT_EQ(moduleConfig.installSteps.installSteps[0].optionalFileGroups.groups[0].plugins.plugins[0].name,
        "Ancient Nord Armors");
}

TEST_F(ModuleConfigurationTest_Xavbio, FirstPluginImagePath)
{
    EXPECT_EQ(moduleConfig.installSteps.installSteps[0].optionalFileGroups.groups[0].plugins.plugins[0].image.path,
        "fomod\\91136-1683833629-107808357_png.jpg");
}

TEST_F(ModuleConfigurationTest_Xavbio, SecondPluginName)
{
    EXPECT_EQ(moduleConfig.installSteps.installSteps[0].optionalFileGroups.groups[0].plugins.plugins[1].name,
        "Imperial Armors");
}

TEST_F(ModuleConfigurationTest_Xavbio, SecondPluginImagePath)
{
    EXPECT_EQ(moduleConfig.installSteps.installSteps[0].optionalFileGroups.groups[0].plugins.plugins[1].image.path,
        "fomod\\86097-1677724584-1488206295_png.jpg");
}

TEST_F(ModuleConfigurationTest_Xavbio, SecondStepVisibleFlag)
{
    EXPECT_EQ(moduleConfig.installSteps.installSteps[1].visible.operatorType, OperatorTypeEnum::AND);
    EXPECT_EQ(moduleConfig.installSteps.installSteps[1].visible.flagDependencies[0].flag, "1");
    EXPECT_EQ(moduleConfig.installSteps.installSteps[1].visible.flagDependencies[0].value, "On");
}

TEST_F(ModuleConfigurationTest_Xavbio, ConditionFlag)
{
    const auto flags = moduleConfig.installSteps.installSteps[0].optionalFileGroups.groups[0].plugins.plugins[0].conditionFlags.flags;
    EXPECT_EQ(flags[0].name, "1");
    EXPECT_EQ(flags[0].value, "On");
}

TEST_F(ModuleConfigurationTest_Xavbio, Folders)
{
    const auto file = moduleConfig.installSteps.installSteps[1].optionalFileGroups.groups[0].plugins.plugins[0].files.files[0];
    EXPECT_EQ(file.source, "1. Ancient Nord Armor\\CalienteTools");
    EXPECT_EQ(file.destination, "CalienteTools");
    EXPECT_EQ(file.priority, 0);
    EXPECT_EQ(file.isFolder, true);
}