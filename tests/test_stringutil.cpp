#include "stringutil.h"
#include <gtest/gtest.h>
#include <QString>

TEST(StringUtil, Trim_Copy)
{
    std::string str = "  extra spaces  ";
    EXPECT_EQ(trim(str), "extra spaces");
}

TEST(StringUtilTests, FormatPluginDescription_UrlConversion)
{
    QString input    = "Check this link: https://example.com";
    QString expected = "Check this link: <a href=\"https://example.com\">https://example.com</a>";
    QString result   = formatPluginDescription(input);
    EXPECT_EQ(result, expected);
}

TEST(StringUtilTests, FormatPluginDescription_LineBreaks)
{
    QString input    = "Line1\nLine2\r\nLine3\rLine4";
    QString expected = "Line1<br>Line2<br>Line3<br>Line4";
    QString result   = formatPluginDescription(input);
    EXPECT_EQ(result, expected);
}

TEST(StringUtilTests, FormatPluginDescription_UrlAndLineBreaks)
{
    QString input    = "Visit https://example.com\nNew line";
    QString expected = "Visit <a href=\"https://example.com\">https://example.com</a><br>New line";
    QString result   = formatPluginDescription(input);
    EXPECT_EQ(result, expected);
}

TEST(StringUtilTests, FormatPluginDescription_MultipleUrls)
{
    QString input    = "First link: https://example.com\nSecond link: http://test.com";
    QString expected =
        "First link: <a href=\"https://example.com\">https://example.com</a><br>Second link: <a href=\"http://test.com\">http://test.com</a>";
    QString result = formatPluginDescription(input);
    EXPECT_EQ(result, expected);
}

TEST(StringUtilTests, FormatPluginDescription_NoUrlsOrLineBreaks)
{
    QString input    = "Just a plain text.";
    QString expected = "Just a plain text.";
    QString result   = formatPluginDescription(input);
    EXPECT_EQ(result, expected);
}

TEST(StringUtilTests, FormatPluginDescription_SpecialCharacters)
{
    QString input =
        "Patch for https://www.afkmods.com/index.php?/files/file/2006-provincial-courier-service/ provided.";
    QString expected =
        "Patch for <a href=\"https://www.afkmods.com/index.php?/files/file/2006-provincial-courier-service/\">https://www.afkmods.com/index.php?/files/file/2006-provincial-courier-service/</a> provided.";
    QString result = formatPluginDescription(input);

    EXPECT_EQ(result, expected);
}

TEST(StringUtilTests, FormatPluginDescription_UrlWithSpecialCharacters)
{
    QString input    = "Check this link: https://example.com/path?query=param&other=param#fragment";
    QString expected = "Check this link: <a href=\"https://example.com/path?query=param&other=param#fragment\">https://example.com/path?query=param&other=param#fragment</a>";
    QString result   = formatPluginDescription(input);
    EXPECT_EQ(result, expected);
}

TEST(StringUtilTests, FormatPluginDescription_UrlWithPort)
{
    QString input    = "Visit http://example.com:8080 for more info.";
    QString expected = "Visit <a href=\"http://example.com:8080\">http://example.com:8080</a> for more info.";
    QString result   = formatPluginDescription(input);
    EXPECT_EQ(result, expected);
}

TEST(StringUtilTests, FormatPluginDescription_UrlWithSubdomain)
{
    QString input    = "Check out https://sub.example.com for details.";
    QString expected = "Check out <a href=\"https://sub.example.com\">https://sub.example.com</a> for details.";
    QString result   = formatPluginDescription(input);
    EXPECT_EQ(result, expected);
}

TEST(StringUtilTests, FormatPluginDescription_UrlWithIP)
{
    QString input    = "Access the server at http://192.168.1.1.";
    QString expected = "Access the server at <a href=\"http://192.168.1.1\">http://192.168.1.1</a>.";
    QString result   = formatPluginDescription(input);
    EXPECT_EQ(result, expected);
}