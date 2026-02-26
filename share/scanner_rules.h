#pragma once

#include "stringutil.h"

#include <QVariant>
#include <string>
#include <vector>

/**
 * Pure decision functions extracted from the scanner and archive parser.
 * These encode the rules for how the scanner interacts with mod settings
 * and archive contents, separated from the MO2 API so they can be tested
 * in isolation.
 */

enum class ScanResult { HAS_FOMOD, NO_FOMOD, NO_ARCHIVE };

enum class SettingAction {
    SetFlag,   // Write "{}" to mark mod as having a FOMOD
    ClearFlag, // Remove the setting (set to 0)
    NoChange   // Leave the setting untouched
};

/**
 * Determine what action the scanner should take on a mod's pluginSetting
 * given the current setting value and the scan result.
 *
 * Critical invariant: rich JSON data (user choices written by the installer)
 * must NEVER be cleared. Only bare "{}" flags set by a previous scan may be cleared.
 */
inline SettingAction determineSettingAction(const QVariant& currentSetting, const ScanResult scanResult)
{
    const bool hasSetting = currentSetting != 0;

    if (!hasSetting && scanResult == ScanResult::HAS_FOMOD) {
        return SettingAction::SetFlag;
    }

    if (hasSetting && scanResult == ScanResult::NO_FOMOD) {
        // Only clear bare flags — never destroy rich JSON written by the installer.
        if (currentSetting.toString() == "{}") {
            return SettingAction::ClearFlag;
        }
        return SettingAction::NoChange;
    }

    return SettingAction::NoChange;
}

/**
 * Check whether a list of archive file paths contains a FOMOD.
 * A valid FOMOD requires ModuleConfig.xml; info.xml is optional.
 */
inline bool hasFomodFilePaths(const std::vector<std::wstring>& filePaths)
{
    for (const auto& path : filePaths) {
        if (endsWithCaseInsensitive(path, std::wstring(StringConstants::FomodFiles::W_MODULE_CONFIG))) {
            return true;
        }
    }
    return false;
}
