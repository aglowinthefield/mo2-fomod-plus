#include "scanner_rules.h"

#include <QVariant>
#include <gtest/gtest.h>

// ============================================================================
// determineSettingAction
// ============================================================================

// --- The regression scenario: scanner must never wipe rich JSON choices ---

TEST(DetermineSettingAction, NeverClearsRichJsonOnNoFomod)
{
    // This is THE bug. A mod has installer-written choices, the archive scan
    // returns NO_FOMOD (e.g. archive deleted, detection failure). The scanner
    // must leave the choices alone.
    const QVariant richJson(R"({"steps":[{"name":"Step 1","groups":[{"name":"Group 1","plugins":["Plugin A"]}]}]})");
    EXPECT_EQ(SettingAction::NoChange, determineSettingAction(richJson, ScanResult::NO_FOMOD));
}

TEST(DetermineSettingAction, NeverClearsRichJsonOnNoArchive)
{
    const QVariant richJson(R"({"steps":[]})");
    EXPECT_EQ(SettingAction::NoChange, determineSettingAction(richJson, ScanResult::NO_ARCHIVE));
}

TEST(DetermineSettingAction, NeverClearsRichJsonOnHasFomod)
{
    const QVariant richJson(R"({"steps":[{"name":"Step","groups":[]}]})");
    EXPECT_EQ(SettingAction::NoChange, determineSettingAction(richJson, ScanResult::HAS_FOMOD));
}

// --- Normal scanner flag lifecycle ---

TEST(DetermineSettingAction, SetsFlagWhenNoSettingAndHasFomod)
{
    const QVariant noSetting(0);
    EXPECT_EQ(SettingAction::SetFlag, determineSettingAction(noSetting, ScanResult::HAS_FOMOD));
}

TEST(DetermineSettingAction, ClearsBareFlag_WhenNoFomod)
{
    const QVariant bareFlag("{}");
    EXPECT_EQ(SettingAction::ClearFlag, determineSettingAction(bareFlag, ScanResult::NO_FOMOD));
}

TEST(DetermineSettingAction, NoChangeWhenNoSettingAndNoFomod)
{
    const QVariant noSetting(0);
    EXPECT_EQ(SettingAction::NoChange, determineSettingAction(noSetting, ScanResult::NO_FOMOD));
}

TEST(DetermineSettingAction, NoChangeWhenNoSettingAndNoArchive)
{
    const QVariant noSetting(0);
    EXPECT_EQ(SettingAction::NoChange, determineSettingAction(noSetting, ScanResult::NO_ARCHIVE));
}

TEST(DetermineSettingAction, NoChangeWhenBareFlagAndHasFomod)
{
    // Already flagged, scan confirms FOMOD exists — nothing to do.
    const QVariant bareFlag("{}");
    EXPECT_EQ(SettingAction::NoChange, determineSettingAction(bareFlag, ScanResult::HAS_FOMOD));
}

TEST(DetermineSettingAction, NoChangeWhenBareFlagAndNoArchive)
{
    // Archive missing — can't confirm or deny. Don't touch the flag.
    const QVariant bareFlag("{}");
    EXPECT_EQ(SettingAction::NoChange, determineSettingAction(bareFlag, ScanResult::NO_ARCHIVE));
}

// --- Edge cases: various shapes of rich JSON ---

TEST(DetermineSettingAction, PreservesMinimalJsonObject)
{
    const QVariant minimalJson(R"({"steps":[]})");
    EXPECT_EQ(SettingAction::NoChange, determineSettingAction(minimalJson, ScanResult::NO_FOMOD));
}

TEST(DetermineSettingAction, PreservesJsonWithWhitespace)
{
    // Unlikely, but a pretty-printed "{}" is NOT a bare flag.
    const QVariant prettyJson("{ }");
    EXPECT_EQ(SettingAction::NoChange, determineSettingAction(prettyJson, ScanResult::NO_FOMOD));
}

TEST(DetermineSettingAction, PreservesLargeRichJson)
{
    const QVariant bigJson(
        R"({"steps":[{"name":"Patches","groups":[{"name":"Compatibility","plugins":["Patch A.esp","Patch B.esp"],"deselected":["Patch C.esp"]}]},{"name":"Textures","groups":[{"name":"Resolution","plugins":["2K Textures"]}]}]})");
    EXPECT_EQ(SettingAction::NoChange, determineSettingAction(bigJson, ScanResult::NO_FOMOD));
}

TEST(DetermineSettingAction, PreservesMalformedJson)
{
    // If the setting is non-zero but garbled, don't destroy it.
    const QVariant garbled("not json at all");
    EXPECT_EQ(SettingAction::NoChange, determineSettingAction(garbled, ScanResult::NO_FOMOD));
}

// ============================================================================
// hasFomodFilePaths
// ============================================================================

TEST(HasFomodFilePaths, ReturnsTrueForModuleConfigOnly)
{
    // This was the other half of the bug: info.xml should NOT be required.
    const std::vector<std::wstring> paths = { L"fomod/ModuleConfig.xml" };
    EXPECT_TRUE(hasFomodFilePaths(paths));
}

TEST(HasFomodFilePaths, ReturnsTrueForBothFiles)
{
    const std::vector<std::wstring> paths = { L"fomod/info.xml", L"fomod/ModuleConfig.xml" };
    EXPECT_TRUE(hasFomodFilePaths(paths));
}

TEST(HasFomodFilePaths, ReturnsFalseForInfoXmlOnly)
{
    const std::vector<std::wstring> paths = { L"fomod/info.xml" };
    EXPECT_FALSE(hasFomodFilePaths(paths));
}

TEST(HasFomodFilePaths, ReturnsFalseForEmptyList)
{
    const std::vector<std::wstring> paths = {};
    EXPECT_FALSE(hasFomodFilePaths(paths));
}

TEST(HasFomodFilePaths, ReturnsFalseForUnrelatedFiles)
{
    const std::vector<std::wstring> paths = { L"meshes/nif01.nif", L"textures/diffuse.dds" };
    EXPECT_FALSE(hasFomodFilePaths(paths));
}

TEST(HasFomodFilePaths, CaseInsensitiveMatch)
{
    const std::vector<std::wstring> paths = { L"FOMOD/MODULECONFIG.XML" };
    EXPECT_TRUE(hasFomodFilePaths(paths));
}

TEST(HasFomodFilePaths, NestedPathStillMatches)
{
    // Some archives have extra nesting: "ModName/fomod/ModuleConfig.xml"
    const std::vector<std::wstring> paths = { L"SomeModName/fomod/ModuleConfig.xml" };
    EXPECT_TRUE(hasFomodFilePaths(paths));
}

TEST(HasFomodFilePaths, SubstringFomodDirStillMatches)
{
    // "notfomod/ModuleConfig.xml" ends with "fomod/ModuleConfig.xml" because
    // the check is a suffix match. This is acceptable — real archives won't
    // have directory names like "notfomod", and a false positive here just
    // means the scanner flags a mod as having a FOMOD (harmless).
    const std::vector<std::wstring> paths = { L"notfomod/ModuleConfig.xml" };
    EXPECT_TRUE(hasFomodFilePaths(paths));
}

TEST(HasFomodFilePaths, WrongDirectoryDoesNotMatch)
{
    // ModuleConfig.xml in a non-fomod directory should not match.
    const std::vector<std::wstring> paths = { L"config/ModuleConfig.xml" };
    EXPECT_FALSE(hasFomodFilePaths(paths));
}
