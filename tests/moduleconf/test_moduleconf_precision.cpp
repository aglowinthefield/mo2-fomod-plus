#include "../../installer/xml/ModuleConfiguration.h"
#include <QString>
#include <filesystem>
#include <gtest/gtest.h>

class ModuleConfPrecision : public ::testing::Test {
  protected:
    ModuleConfiguration moduleConfig;

    void SetUp() override
    {
        const std::string filePath
            = (std::filesystem::path(__FILE__).parent_path() / "test_moduleconf_precision.xml").string();
        moduleConfig.deserialize(QString::fromStdString(filePath));
    }
};

TEST_F(ModuleConfPrecision, ModuleName) { EXPECT_EQ(moduleConfig.moduleName, "Precision"); }

TEST_F(ModuleConfPrecision, RequiredInstallFiles)
{
    const auto requiredInstallFiles = moduleConfig.requiredInstallFiles.files;
    EXPECT_EQ(requiredInstallFiles.size(), 8);
    EXPECT_EQ(requiredInstallFiles[0].source, "Interface");
    EXPECT_EQ(requiredInstallFiles[0].destination, "Interface");
    EXPECT_EQ(requiredInstallFiles[7].source, "Precision.esp");
    EXPECT_EQ(requiredInstallFiles[7].destination, "Precision.esp");
}

TEST_F(ModuleConfPrecision, PatternDependencies)
{
    const auto tkDodge = moduleConfig.installSteps.installSteps[0].optionalFileGroups.groups[0].plugins.plugins[0];

    const auto typeDescriptor = tkDodge.typeDescriptor;
    EXPECT_EQ(typeDescriptor.dependencyType.defaultType, PluginTypeEnum::NotUsable);

    const auto pattern = typeDescriptor.dependencyType.patterns.patterns[0];
    EXPECT_EQ(pattern.type, PluginTypeEnum::Recommended);

    // TODO: This doesn't QUITE add up to how the xml is formatted. fine for now.
    EXPECT_EQ(pattern.dependencies.operatorType, OperatorTypeEnum::OR);
    EXPECT_EQ(pattern.dependencies.fileDependencies.size(), 4);
    EXPECT_EQ(pattern.dependencies.flagDependencies.size(), 0);
    // <fileDependency file="TKDodge.esp" state="Active"/>
    // <fileDependency file="TKDodge.esp" state="Inactive"/>
    // <fileDependency file="UltimateCombat.esp" state="Active"/>
    // <fileDependency file="UltimateCombat.esp" state="Inactive"/>
    EXPECT_EQ(pattern.dependencies.fileDependencies[0].file, "TKDodge.esp");
    EXPECT_EQ(pattern.dependencies.fileDependencies[0].state, FileDependencyTypeEnum::Active);
    EXPECT_EQ(pattern.dependencies.fileDependencies[1].file, "TKDodge.esp");
    EXPECT_EQ(pattern.dependencies.fileDependencies[1].state, FileDependencyTypeEnum::Inactive);
    EXPECT_EQ(pattern.dependencies.fileDependencies[2].file, "UltimateCombat.esp");
    EXPECT_EQ(pattern.dependencies.fileDependencies[2].state, FileDependencyTypeEnum::Active);
    EXPECT_EQ(pattern.dependencies.fileDependencies[3].file, "UltimateCombat.esp");
    EXPECT_EQ(pattern.dependencies.fileDependencies[3].state, FileDependencyTypeEnum::Inactive);
    // <files>
    //     <folder source="Compatibility\TK Dodge Ultimate Combat" destination="Nemesis_Engine" priority="0" />
    // </files>
    EXPECT_EQ(tkDodge.files.files[0].source, "Compatibility\\TK Dodge Ultimate Combat");
    EXPECT_EQ(tkDodge.files.files[0].destination, "Nemesis_Engine");
    EXPECT_EQ(tkDodge.files.files[0].priority, 0);
}
