#ifndef STRINGCONSTANTS_H
#define STRINGCONSTANTS_H
#include <algorithm>
#include <regex>
#include <string>
#include <vector>
#include <QString>


namespace StringConstants {
namespace Plugin {
    constexpr std::string_view NAME           = "FOMOD Plus";
    constexpr std::string_view AUTHOR         = "clearing";
    constexpr std::string_view DESCRIPTION    = "Extends the capabilities of the FOMOD installer for advanced users.";
    constexpr std::wstring_view W_NAME        = L"FOMOD Plus";
    constexpr std::wstring_view W_AUTHOR      = L"clearing";
    constexpr std::wstring_view W_DESCRIPTION = L"Extends the capabilities of the FOMOD installer for advanced users.";
}

namespace FomodFiles {
    constexpr std::string_view FOMOD_DIR        = "fomod";
    constexpr std::string_view INFO_XML         = "info.xml";
    constexpr std::string_view MODULE_CONFIG    = "ModuleConfig.xml";
    constexpr std::wstring_view W_FOMOD_DIR     = L"fomod";
    constexpr std::wstring_view W_INFO_XML      = L"info.xml";
    constexpr std::wstring_view W_MODULE_CONFIG = L"ModuleConfig.xml";

    constexpr std::string_view TYPE_REQUIRED        = "Required";
    constexpr std::string_view TYPE_OPTIONAL        = "Optional";
    constexpr std::string_view TYPE_RECOMMENDED     = "Recommended";
    constexpr std::string_view TYPE_NOT_USABLE      = "NotUsable";
    constexpr std::string_view TYPE_COULD_BE_USABLE = "CouldBeUsable";
}
}

// trim from start (in place)
inline void ltrim(std::string& s)
{
    s.erase(s.begin(), std::ranges::find_if(s, [](const unsigned char ch) {
        return !std::isspace(ch);
    }));
}

// trim from end (in place)
inline void rtrim(std::string& s)
{
    s.erase(std::find_if(s.rbegin(), s.rend(), [](const unsigned char ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

// trim from both ends (in place)
inline std::string& trim(std::string& s)
{
    ltrim(s);
    rtrim(s);
    return s;
}

inline void trim(const std::vector<std::string>& strings)
{
    for (auto s : strings) { trim(s); }
}

inline std::wstring toLower(const std::wstring& str)
{
    std::wstring lowerStr = str;
    std::ranges::transform(lowerStr, lowerStr.begin(), towlower);
    return lowerStr;
}

inline std::string toLower(const std::string& str)
{
    std::string lowerStr = str;
    std::ranges::transform(lowerStr, lowerStr.begin(), tolower);
    return lowerStr;
}

inline bool endsWithCaseInsensitive(const std::wstring& str, const std::wstring& suffix)
{
    const std::wstring lowerStr = toLower(str);
    if (const std::wstring lowerSuffix = toLower(suffix); lowerStr.length() >= lowerSuffix.length()) {
        return 0 == lowerStr.compare(lowerStr.length() - lowerSuffix.length(), lowerSuffix.length(), lowerSuffix);
    }
    return false;
}

inline QString formatPluginDescription(const QString& text)
{
    std::string formattedText = text.toStdString();
    // Replace URLs with <a href> tags
    const std::regex urlRegex(R"((https?://[^\s]+))");
    formattedText = std::regex_replace(formattedText, urlRegex, R"(<a href="$&">$&</a>)");

    // Replace line breaks
    formattedText = std::regex_replace(formattedText, std::regex("&#13;&#10;"), "<br>");
    formattedText = std::regex_replace(formattedText, std::regex("\\r\\n"), "<br>");
    formattedText = std::regex_replace(formattedText, std::regex("\\r"), "<br>");
    formattedText = std::regex_replace(formattedText, std::regex("\\n"), "<br>");


    return QString::fromStdString(formattedText);
}




#endif