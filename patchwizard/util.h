#ifndef UTIL_H
#define UTIL_H

struct FomodNotes {
    QString modName;
    QVector<QString> hasPatchFor;
    QVector<QString> installedPatchFor;
    QVector<QString> notInstalledPatchFor;
};

struct ModWithPatches {
    MOBase::IModInterface* modPtr;
    FomodNotes fomodNotes;
};

using PluginToMentionsMap = std::unordered_map<QString, std::vector<ModWithPatches>>;



#endif //UTIL_H
