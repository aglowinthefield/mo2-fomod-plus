#ifndef UTIL_H
#define UTIL_H

using PluginToMentionsMap = std::unordered_map<QString, std::vector<MOBase::IModInterface*>>;

struct FomodNotes {
    QString modName;
    QVector<QString> hasPatchFor;
    QVector<QString> installedPatchFor;
    QVector<QString> notInstalledPatchFor;
};

#endif //UTIL_H
