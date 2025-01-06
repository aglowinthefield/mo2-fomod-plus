#ifndef STRINGCONSTANTS_H
#define STRINGCONSTANTS_H

namespace StringConstants {
    namespace Plugin {
        constexpr auto NAME = "FomodPlus";
        constexpr auto AUTHOR = "clearing";
        constexpr auto DESCRIPTION = "FomodPlus is a plugin for Mod Organizer 2 that extends the functionality of the FOMOD Installer.";
    }

    namespace FomodFiles {
        constexpr auto FOMOD_DIR = "fomod";
        constexpr auto INFO_XML = "info.xml";
        constexpr auto MODULE_CONFIG = "ModuleConfig.xml";

        constexpr auto TYPE_REQUIRED = "Required";
        constexpr auto TYPE_OPTIONAL = "Optional";
        constexpr auto TYPE_RECOMMENDED = "Recommended";
        constexpr auto TYPE_NOT_USABLE = "NotUsable";
        constexpr auto TYPE_COULD_BE_USABLE = "CouldBeUsable";
    }

    // Add more nested namespaces as needed
}

#endif //STRINGCONSTANTS_H
