#include "../../installer/xml/ModuleConfiguration.h"
#include <QString>
#include <filesystem>
#include <gtest/gtest.h>

class MCCityTrees : public testing::Test {
  protected:
    ModuleConfiguration moduleConfig;

    void SetUp() override
    {
        const std::string filePath
            = (std::filesystem::path(__FILE__).parent_path() / "test_moduleconf_citytrees.xml").string();
        moduleConfig.deserialize(QString::fromStdString(filePath));
    }
};

TEST_F(MCCityTrees, Deserializes) { EXPECT_EQ(moduleConfig.moduleName, "RogueUnicorn - City Trees"); }

// 521 conditionalFileInstalls->patterns->pattern. thank you intellij xpath
// done via count(//conditionalFileInstalls/patterns/pattern)
TEST_F(MCCityTrees, ConditionalFileInstalls)
{
    EXPECT_EQ(moduleConfig.conditionalFileInstalls.patterns.size(), 521);

    const auto firstConditional = moduleConfig.conditionalFileInstalls.patterns[0];

    EXPECT_EQ(firstConditional.dependencies.operatorType, OperatorTypeEnum::AND);
    EXPECT_EQ(firstConditional.dependencies.fileDependencies.size(), 0);
    EXPECT_EQ(firstConditional.dependencies.flagDependencies.size(), 3);
    EXPECT_EQ(firstConditional.dependencies.flagDependencies[0].flag, "dawnstar");
    EXPECT_EQ(firstConditional.dependencies.flagDependencies[0].value, "1");

    EXPECT_EQ(firstConditional.files.files[0].source, "_addon\\treeswaps\\notwl_Dawnstar_A1.esp");
    EXPECT_EQ(firstConditional.files.files[0].destination, "CT_10_Dawnstar_A1.esp");
    EXPECT_EQ(firstConditional.files.files[0].priority, 0);
}

TEST_F(MCCityTrees, VisibleDependencies)
{
    const auto visibilityStep = moduleConfig.installSteps.installSteps.at(5);
    const auto visibility     = visibilityStep.visible.flagDependencies;
    EXPECT_EQ(visibility.size(), 1);
    EXPECT_EQ(visibility.front().flag, "addon1menu");
    EXPECT_EQ(visibility.front().value, "1");
}
