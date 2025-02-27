#include <gtest/gtest.h>
#include "../../installer/xml/ModuleConfiguration.h"
#include <QString>
#include <filesystem>

class MCBos : public testing::Test {
protected:
    ModuleConfiguration moduleConfig;

    void SetUp() override
    {
        const std::string filePath = (std::filesystem::path(__FILE__).parent_path() / "test_moduleconf_bos.xml").
            string();
        moduleConfig.deserialize(QString::fromStdString(filePath));
    }
};

TEST_F(MCBos, Deserializes)
{
    EXPECT_EQ(moduleConfig.moduleName, "Base Object Swapper");
}

TEST_F(MCBos, VersionDependency)
{
    const auto firstGroup  = moduleConfig.installSteps.installSteps.front().optionalFileGroups.groups.front();
    const auto firstPlugin = firstGroup.plugins.plugins.front();

    EXPECT_EQ(firstPlugin.typeDescriptor.dependencyType.defaultType, PluginTypeEnum::Optional);

    const auto dependencies = firstPlugin.typeDescriptor.dependencyType.patterns.patterns;
    EXPECT_EQ(dependencies.size(), 3);

    EXPECT_EQ(dependencies.front().dependencies.gameDependencies.size(), 1);

    const auto firstGameDep = dependencies.front().dependencies.gameDependencies.front();
    EXPECT_EQ(firstGameDep.version, "1.6.1130.0");

}