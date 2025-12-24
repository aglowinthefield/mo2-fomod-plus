#pragma once

#include <QString>
#include <map>

namespace UiColors {

enum class ColorApplication {
    BACKGROUND,
    BORDER,
    TEXT,
    ALL
};

// Color values
namespace Colors {
    // Light
    const QString Light0 = "251, 241, 199";
    const QString Light1 = "235, 219, 178";
    const QString Light2 = "213, 196, 161";
    const QString Light3 = "189, 174, 147";

    // Dark
    const QString Dark0 = "40, 40, 40";
    const QString Dark1 = "60, 56, 54";
    const QString Dark2 = "80, 73, 69";
    const QString Dark3 = "102, 92, 84";

    // Red
    const QString Red       = "204, 36, 29";
    const QString RedBright = "251, 73, 52";

    // Green
    const QString Green       = "152, 151, 26";
    const QString GreenBright = "184, 187, 38";

    // Yellow
    const QString Yellow       = "215, 153, 33";
    const QString YellowBright = "250, 189, 47";

    // Blue
    const QString Blue       = "69, 133, 136";
    const QString BlueBright = "131, 165, 152";

    // Purple
    const QString Purple       = "177, 98, 134";
    const QString PurpleBright = "211, 134, 155";

    // Aqua
    const QString Aqua       = "104, 157, 106";
    const QString AquaBright = "142, 192, 124";

    // Orange
    const QString Orange       = "214, 93, 14";
    const QString OrangeBright = "254, 128, 25";
}

// Helper function to generate style strings based on color and application
inline QString generateStyle(const QString& color, const ColorApplication application, const float opacity = 0.4,
    const int borderWidth                                                                                  = 1)
{
    QString style;

    switch (application) {
    case ColorApplication::BACKGROUND:
        style = QString("QCheckBox { background-color: rgba(%1, %2); } "
                "QRadioButton { background-color: rgba(%1, %2); }")
            .arg(color).arg(opacity);
        break;

    case ColorApplication::BORDER:
        style = QString("QCheckBox { border: %1px dashed rgb(%2); } "
                "QRadioButton { border: %1px dashed rgb(%2); }")
            .arg(borderWidth).arg(color);
        break;

    case ColorApplication::TEXT:
        style = QString("QCheckBox { color: rgb(%1); } "
                "QRadioButton { color: rgb(%1); }")
            .arg(color);
        break;

    case ColorApplication::ALL:
        style = QString("QCheckBox { background-color: rgba(%1, %2); border: %3px solid rgb(%1); color: rgb(%1); } "
                "QRadioButton { background-color: rgba(%1, %2); border: %3px solid rgb(%1); color: rgb(%1); }")
            .arg(color).arg(opacity).arg(borderWidth);
        break;
    }

    return style;
}

// Main function to get style for a color name and application
inline QString getStyle(const QString& colorName, const ColorApplication application = ColorApplication::BACKGROUND,
    const float opacity                                                              = 0.4, const int borderWidth = 1)
{
    static const std::map<QString, QString> colorValues = {
        { "Light0", Colors::Light0 },
        { "Light1", Colors::Light1 },
        { "Light2", Colors::Light2 },
        { "Light3", Colors::Light3 },
        { "Dark0", Colors::Dark0 },
        { "Dark1", Colors::Dark1 },
        { "Dark2", Colors::Dark2 },
        { "Dark3", Colors::Dark3 },
        { "Red", Colors::Red },
        { "Red Bright", Colors::RedBright },
        { "Green", Colors::Green },
        { "Green Bright", Colors::GreenBright },
        { "Yellow", Colors::Yellow },
        { "Yellow Bright", Colors::YellowBright },
        { "Blue", Colors::Blue },
        { "Blue Bright", Colors::BlueBright },
        { "Purple", Colors::Purple },
        { "Purple Bright", Colors::PurpleBright },
        { "Aqua", Colors::Aqua },
        { "Aqua Bright", Colors::AquaBright },
        { "Orange", Colors::Orange },
        { "Orange Bright", Colors::OrangeBright }
    };

    if (const auto it = colorValues.find(colorName); it != colorValues.end()) {
        return generateStyle(it->second, application, opacity, borderWidth);
    }

    return {};
}

// For backward compatibility
const static std::map<QString, QString> colorStyles = {
    { "Light0", getStyle("Light0", ColorApplication::BACKGROUND) },
    { "Light1", getStyle("Light1", ColorApplication::BACKGROUND) },
    { "Light2", getStyle("Light2", ColorApplication::BACKGROUND) },
    { "Light3", getStyle("Light3", ColorApplication::BACKGROUND) },
    { "Dark0", getStyle("Dark0", ColorApplication::BACKGROUND) },
    { "Dark1", getStyle("Dark1", ColorApplication::BACKGROUND) },
    { "Dark2", getStyle("Dark2", ColorApplication::BACKGROUND) },
    { "Dark3", getStyle("Dark3", ColorApplication::BACKGROUND) },
    { "Red", getStyle("Red", ColorApplication::BACKGROUND) },
    { "Red Bright", getStyle("Red Bright", ColorApplication::BACKGROUND) },
    { "Green", getStyle("Green", ColorApplication::BACKGROUND) },
    { "Green Bright", getStyle("Green Bright", ColorApplication::BACKGROUND) },
    { "Yellow", getStyle("Yellow", ColorApplication::BACKGROUND) },
    { "Yellow Bright", getStyle("Yellow Bright", ColorApplication::BACKGROUND) },
    { "Blue", getStyle("Blue", ColorApplication::BACKGROUND) },
    { "Blue Bright", getStyle("Blue Bright", ColorApplication::BACKGROUND) },
    { "Purple", getStyle("Purple", ColorApplication::BACKGROUND) },
    { "Purple Bright", getStyle("Purple Bright", ColorApplication::BACKGROUND) },
    { "Aqua", getStyle("Aqua", ColorApplication::BACKGROUND) },
    { "Aqua Bright", getStyle("Aqua Bright", ColorApplication::BACKGROUND) },
    { "Orange", getStyle("Orange", ColorApplication::BACKGROUND) },
    { "Orange Bright", getStyle("Orange Bright", ColorApplication::BACKGROUND) }
};

}
