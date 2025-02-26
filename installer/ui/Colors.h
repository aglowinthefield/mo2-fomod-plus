#ifndef COLORS_H
#define COLORS_H
#include <QString>
#include <map>

namespace UiColors {
// Light
const auto styleGruvboxLight0 =
    "QCheckBox { background-color: rgba(251, 241, 199, 0.4); } QRadioButton { background-color: rgba(251, 241, 199, 0.4); }";
const auto styleGruvboxLight1 =
    "QCheckBox { background-color: rgba(235, 219, 178, 0.4); } QRadioButton { background-color: rgba(235, 219, 178, 0.4); }";
const auto styleGruvboxLight2 =
    "QCheckBox { background-color: rgba(213, 196, 161, 0.4); } QRadioButton { background-color: rgba(213, 196, 161, 0.4); }";
const auto styleGruvboxLight3 =
    "QCheckBox { background-color: rgba(189, 174, 147, 0.4); } QRadioButton { background-color: rgba(189, 174, 147, 0.4); }";

// Dark
const auto styleGruvboxDark0 =
    "QCheckBox { background-color: rgba(40, 40, 40, 0.4); } QRadioButton { background-color: rgba(40, 40, 40, 0.4); }";
const auto styleGruvboxDark1 =
    "QCheckBox { background-color: rgba(60, 56, 54, 0.4); } QRadioButton { background-color: rgba(60, 56, 54, 0.4); }";
const auto styleGruvboxDark2 =
    "QCheckBox { background-color: rgba(80, 73, 69, 0.4); } QRadioButton { background-color: rgba(80, 73, 69, 0.4); }";
const auto styleGruvboxDark3 =
    "QCheckBox { background-color: rgba(102, 92, 84, 0.4); } QRadioButton { background-color: rgba(102, 92, 84, 0.4); }";

// Red
const auto styleGruvboxRed =
    "QCheckBox { background-color: rgba(204, 36, 29, 0.4); } QRadioButton { background-color: rgba(204, 36, 29, 0.4); }";
const auto styleGruvboxRedBr =
    "QCheckBox { background-color: rgba(251, 73, 52, 0.4); } QRadioButton { background-color: rgba(251, 73, 52, 0.4); }";

// Green
const auto styleGruvboxGreen =
    "QCheckBox { background-color: rgba(152, 151, 26, 0.4); } QRadioButton { background-color: rgba(152, 151, 26, 0.4); }";
const auto styleGruvboxGreenBr =
    "QCheckBox { background-color: rgba(184, 187, 38, 0.4); } QRadioButton { background-color: rgba(184, 187, 38, 0.4); }";

// Yellow
const auto styleGruvboxYellow =
    "QCheckBox { background-color: rgba(215, 153, 33, 0.4); } QRadioButton { background-color: rgba(215, 153, 33, 0.4); }";
const auto styleGruvboxYellowBr =
    "QCheckBox { background-color: rgba(250, 189, 47, 0.4); } QRadioButton { background-color: rgba(250, 189, 47, 0.4); }";

// Blue
const auto styleGruvboxBlue =
    "QCheckBox { background-color: rgba(69, 133, 136, 0.4); } QRadioButton { background-color: rgba(69, 133, 136, 0.4); }";
const auto styleGruvboxBlueBr =
    "QCheckBox { background-color: rgba(131, 165, 152, 0.4); } QRadioButton { background-color: rgba(131, 165, 152, 0.4); }";

// Purple
const auto styleGruvboxPurple =
    "QCheckBox { background-color: rgba(177, 98, 134, 0.4); } QRadioButton { background-color: rgba(177, 98, 134, 0.4); }";
const auto styleGruvboxPurpleBr =
    "QCheckBox { background-color: rgba(211, 134, 155, 0.4); } QRadioButton { background-color: rgba(211, 134, 155, 0.4); }";

// Aqua
const auto styleGruvboxAqua =
    "QCheckBox { background-color: rgba(104, 157, 106, 0.4); } QRadioButton { background-color: rgba(104, 157, 106, 0.4); }";
const auto styleGruvboxAquaBr =
    "QCheckBox { background-color: rgba(142, 192, 124, 0.4); } QRadioButton { background-color: rgba(142, 192, 124, 0.4); }";

// Orange
const auto styleGruvboxOrange =
    "QCheckBox { background-color: rgba(214, 93, 14, 0.4); } QRadioButton { background-color: rgba(214, 93, 14, 0.4); }";
const auto styleGruvboxOrangeBr =
    "QCheckBox { background-color: rgba(254, 128, 25, 0.4); } QRadioButton { background-color: rgba(254, 128, 25, 0.4); }";

const static std::map<QString, QString> colorStyles = {
    { "Light0", styleGruvboxLight0 },
    { "Light1", styleGruvboxLight1 },
    { "Light2", styleGruvboxLight2 },
    { "Light3", styleGruvboxLight3 },
    { "Dark0", styleGruvboxDark0 },
    { "Dark1", styleGruvboxDark1 },
    { "Dark2", styleGruvboxDark2 },
    { "Dark3", styleGruvboxDark3 },
    { "Red", styleGruvboxRed },
    { "Red Bright", styleGruvboxRedBr },
    { "Green", styleGruvboxGreen },
    { "Green Bright", styleGruvboxGreenBr },
    { "Yellow", styleGruvboxYellow },
    { "Yellow Bright", styleGruvboxYellowBr },
    { "Blue", styleGruvboxBlue },
    { "Blue Bright", styleGruvboxBlueBr },
    { "Purple", styleGruvboxPurple },
    { "Purple Bright", styleGruvboxPurpleBr },
    { "Aqua", styleGruvboxAqua },
    { "Aqua Bright", styleGruvboxAquaBr },
    { "Orange", styleGruvboxOrange },
    { "Orange Bright", styleGruvboxOrangeBr }
};

}

#endif //COLORS_H