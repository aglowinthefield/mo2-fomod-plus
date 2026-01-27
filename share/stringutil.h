#ifndef STRINGCONSTANTS_H
#define STRINGCONSTANTS_H
#include <algorithm>
#include <regex>
#include <string>
#include <vector>
#include <QString>


namespace StringConstants
{
    namespace Plugin
    {
        constexpr std::string_view NAME = "FOMOD Plus";
        constexpr std::string_view AUTHOR = "clearing";
        constexpr std::string_view DESCRIPTION =
            "Extends the capabilities of the FOMOD installer for advanced users.\n\n"
            "Available colors (enter exactly): \n"
            "'Light0'\t'Light1'\t'Light2'\t'Light3'\n"
            "'Dark0'\t'Dark1'\t'Dark2'\t'Dark3'\n"
            "'Red'\t'Red Bright'\n"
            "'Green'\t'Green Bright'\n"
            "'Yellow'\t'Yellow Bright'\n"
            "'Blue'\t'Blue Bright'\n"
            "'Purple'\t'Purple Bright'\n"
            "'Aqua'\t'Aqua Bright'\n"
            "'Orange'\t'Orange Bright'\n";
        constexpr std::wstring_view W_NAME = L"FOMOD Plus";
        constexpr std::wstring_view W_AUTHOR = L"clearing";
        constexpr std::wstring_view W_DESCRIPTION =
            L"Extends the capabilities of the FOMOD installer for advanced users.";
    }

    namespace FomodFiles
    {
        constexpr std::string_view FOMOD_DIR = "fomod";
        constexpr std::string_view INFO_XML = "info.xml";
        constexpr std::string_view MODULE_CONFIG = "ModuleConfig.xml";

        // Wide string versions for archive API
        constexpr std::wstring_view W_FOMOD_DIR = L"fomod";
        constexpr std::wstring_view W_INFO_XML = L"fomod/info.xml";
        constexpr std::wstring_view W_MODULE_CONFIG = L"fomod/ModuleConfig.xml";

        constexpr std::string_view TYPE_REQUIRED = "Required";
        constexpr std::string_view TYPE_OPTIONAL = "Optional";
        constexpr std::string_view TYPE_RECOMMENDED = "Recommended";
        constexpr std::string_view TYPE_NOT_USABLE = "NotUsable";
        constexpr std::string_view TYPE_COULD_BE_USABLE = "CouldBeUsable";
    }
}

// Convert narrow string_view to wstring (for ASCII strings only)
inline std::wstring toWide(std::string_view sv)
{
    return std::wstring(sv.begin(), sv.end());
}

// trim from start (in place)
inline void ltrim(std::string& s)
{
    s.erase(s.begin(), std::ranges::find_if(s, [](const unsigned char ch)
    {
        return !std::isspace(ch);
    }));
}

// trim from end (in place)
inline void rtrim(std::string& s)
{
    s.erase(std::find_if(s.rbegin(), s.rend(), [](const unsigned char ch)
    {
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
    if (const std::wstring lowerSuffix = toLower(suffix); lowerStr.length() >= lowerSuffix.length())
    {
        return 0 == lowerStr.compare(lowerStr.length() - lowerSuffix.length(), lowerSuffix.length(), lowerSuffix);
    }
    return false;
}

inline QString formatPluginDescription(const QString& text)
{
    std::string formattedText = text.toStdString();
    // Replace URLs with <a href> tags
    const std::regex
        urlRegex(R"((http|ftp|https):\/\/([\w_-]+(?:(?:\.[\w_-]+)+))([\w.,@?^=%&:\/~+#-]*[\w@?^=%&\/~+#-]))");
    formattedText = std::regex_replace(formattedText, urlRegex, R"(<a href="$&">$&</a>)");

    // Replace line breaks
    formattedText = std::regex_replace(formattedText, std::regex("&#13;&#10;"), "<br>");
    formattedText = std::regex_replace(formattedText, std::regex("\\r\\n"), "<br>");
    formattedText = std::regex_replace(formattedText, std::regex("\\r"), "<br>");
    formattedText = std::regex_replace(formattedText, std::regex("\\n"), "<br>");

    return QString::fromStdString(formattedText);
}

// NOTE: This isn't perfect. Sometimes we have whole filenames, sometimes we're just passing
// the suffix. It should be fine as long as no one names a file like..."Testesl". Idk what that
// would do anyway.
inline bool isPluginFile(const QString& file)
{
    return file.toLower().endsWith("esl")
        || file.toLower().endsWith("esp")
        || file.toLower().endsWith("esm");
}

inline bool isPluginFile(const std::string& file)
{
    const auto lower = toLower(file);
    return lower.ends_with("esl")
        || lower.ends_with("esp")
        || lower.ends_with("esm");
}


#endif
