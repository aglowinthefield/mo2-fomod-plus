#include "xml/ModuleConfiguration.h"
#include <QString>
#include <filesystem>
#include <gtest/gtest.h>

class MCEmbers : public testing::Test {
  protected:
    ModuleConfiguration moduleConfig;

    void SetUp() override
    {
        const std::string filePath
            = (std::filesystem::path(__FILE__).parent_path() / "test_moduleconf_embers.xml").string();
        moduleConfig.deserialize(QString::fromStdString(filePath));
    }
};

TEST_F(MCEmbers, NestedDependencies)
{
    const auto step   = moduleConfig.installSteps.installSteps[3]; // further custom
    const auto group  = step.optionalFileGroups.groups[0]; // further custom
    const auto plugin = group.plugins.plugins[0]; // further custom
    const auto deps   = plugin.typeDescriptor.dependencyType.patterns.patterns[0];
    EXPECT_EQ(1, deps.dependencies.flagDependencies.size());
    EXPECT_EQ(1, deps.dependencies.nestedDependencies.size());

    const auto nestedDep = deps.dependencies.nestedDependencies[0];
    EXPECT_EQ(2, nestedDep.flagDependencies.size());
}