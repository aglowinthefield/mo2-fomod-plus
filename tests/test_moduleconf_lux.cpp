﻿#include <gtest/gtest.h>
#include <pugixml.hpp>
#include <filesystem>
#include "../src/xml/ModuleConfiguration.h"

class ModuleConfigurationTest_Lux : public ::testing::Test {
protected:
    void SetUp() override {
        // Load the XML file
    std::string filePath = (std::filesystem::path(__FILE__).parent_path() / "test_moduleconf_lux.xml").string();
    pugi::xml_parse_result result = doc.load_file(filePath.c_str());
    ASSERT_TRUE(result) << "Failed to load XML file: " << result.description();
    configNode = doc.child("config");
    ASSERT_TRUE(configNode) << "No <config> node found";
    }

    pugi::xml_document doc;
    pugi::xml_node configNode;
};

TEST_F(ModuleConfigurationTest_Lux, DeserializeModuleConfiguration) {
    ModuleConfiguration moduleConfig;
    std::string filePath = (std::filesystem::path(__FILE__).parent_path() / "test_moduleconf_lux.xml").string();
    ASSERT_TRUE(moduleConfig.deserialize(filePath));

    EXPECT_EQ(moduleConfig.moduleName, "Lux (patch hub)");
    EXPECT_EQ(moduleConfig.moduleImage.path, "fomod\\Screenshots\\Header LUX patch hub.jpg");
    EXPECT_EQ(moduleConfig.installSteps.order, "Explicit");

    // Check the first install step
    ASSERT_EQ(moduleConfig.installSteps.installSteps.size(), 5);
    const InstallStep &firstStep = moduleConfig.installSteps.installSteps[0];
    EXPECT_EQ(firstStep.name, "Authors collections of mods");

    // Check the first group in the first install step
    ASSERT_EQ(firstStep.optionalFileGroups.groups.size(), 46); // yep. i counted this.
    const Group &firstGroup = firstStep.optionalFileGroups.groups[0];
    EXPECT_EQ(firstGroup.name, "4thUnknown");
    EXPECT_EQ(firstGroup.type, "SelectAny");

    // Check the first plugin in the first group
    ASSERT_EQ(firstGroup.plugins.plugins.size(), 2); // Adjust this based on actual data
}

TEST_F(ModuleConfigurationTest_Lux, DeserializePlugin) {
    Plugin plugin;
    pugi::xml_node pluginNode = configNode.child("installSteps").child("installStep").child("optionalFileGroups").child("group").child("plugins").child("plugin");
    ASSERT_TRUE(pluginNode);
    ASSERT_TRUE(plugin.deserialize(pluginNode));

    EXPECT_EQ(plugin.name, "Fort Windpoint"); // Replace with actual expected name
    EXPECT_EQ(plugin.description, "Compatibility patch for Fort Windpoint (Credits to AgentW) (ESPFE) Fixed version by Xtudo is required."); // Replace with actual expected description
    EXPECT_EQ(plugin.image.path, ""); // lux has no plugin images
}

TEST_F(ModuleConfigurationTest_Lux, DeserializeGroup) {
    Group group;
    pugi::xml_node groupNode = configNode.child("installSteps").child("installStep").child("optionalFileGroups").child("group");
    ASSERT_TRUE(groupNode);
    ASSERT_TRUE(group.deserialize(groupNode));

    EXPECT_EQ(group.name, "4thUnknown");
    EXPECT_EQ(group.type, "SelectAny");
    EXPECT_EQ(group.plugins.order, "Explicit");
}

TEST_F(ModuleConfigurationTest_Lux, DeserializeInstallStep) {
    InstallStep installStep;
    pugi::xml_node installStepNode = configNode.child("installSteps").child("installStep");
    ASSERT_TRUE(installStepNode);
    ASSERT_TRUE(installStep.deserialize(installStepNode));

    EXPECT_EQ(installStep.name, "Authors collections of mods");
    EXPECT_EQ(installStep.optionalFileGroups.order, "Explicit");
}

TEST_F(ModuleConfigurationTest_Lux, DeserializeDependencies) {
    pugi::xml_node dependencyNode = configNode.child("installSteps").child("installStep").child("optionalFileGroups").child("group").child("plugins").child("plugin").child("typeDescriptor").child("dependencyType").child("patterns").child("pattern").child("dependencies");
    ASSERT_TRUE(dependencyNode) << "No <dependencies> node found";

    std::string operatorType = dependencyNode.attribute("operator").as_string();
    EXPECT_EQ(operatorType, "And"); // Replace with actual expected operator

    pugi::xml_node fileDependencyNode = dependencyNode.child("fileDependency");
    ASSERT_TRUE(fileDependencyNode) << "No <fileDependency> node found";

    std::string file = fileDependencyNode.attribute("file").as_string();
    std::string state = fileDependencyNode.attribute("state").as_string();
    EXPECT_EQ(file, "1SixthHouse.esp"); // Replace with actual expected file
    EXPECT_EQ(state, "Active"); // Replace with actual expected state
}
