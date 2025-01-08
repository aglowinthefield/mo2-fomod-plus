#ifndef STRINGCONSTANTS_H
#define STRINGCONSTANTS_H
#include <string>
#include <vector>

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

// trim from start (in place)
inline void ltrim(std::string &s) {
  s.erase(s.begin(), std::ranges::find_if(s, [](unsigned char ch) {
      return !std::isspace(ch);
  }));
}

// trim from end (in place)
inline void rtrim(std::string &s) {
  s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
      return !std::isspace(ch);
  }).base(), s.end());
}

// trim from both ends (in place)
inline std::string& trim(std::string &s) {
  ltrim(s);
  rtrim(s);
  return s;
}

inline void trim(const std::vector<std::string> &strings) {
  for (auto s : strings) { trim(s); }
}

// // trim from start (copying)
// static std::string ltrim_copy(std::string s) {
//   s.erase(s.begin(), std::ranges::find_if(s, [](unsigned char ch) {
//       return !std::isspace(ch);
//   }));
//   return s;
// }
//
// // trim from end (copying)
// static std::string rtrim_copy(std::string s) {
//   s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
//       return !std::isspace(ch);
//   }).base(), s.end());
//   return s;
// }
//
// // trim from both ends (copying)
// static std::string trim_copy(std::string s) {
//   return ltrim_copy(rtrim_copy(std::move(s)));
// }

#endif //STRINGCONSTANTS_H
