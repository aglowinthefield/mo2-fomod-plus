#ifndef MODLISTITEMMODEL_H
#define MODLISTITEMMODEL_H

#include <QAbstractItemModel>
#include <imodinterface.h>
#include "util.h"

#include <ranges>

struct ModListItem {
    MOBase::IModInterface* modPtr;
    std::vector<QString> pluginNames;
    PluginToMentionsMap pluginToMentionsMap;

    ModListItem(MOBase::IModInterface* mod, const std::vector<QString>& plugins, const PluginToMentionsMap& mentions)
        : modPtr(mod), pluginNames(plugins), pluginToMentionsMap(mentions) {}

};

class ModListItemModel final : public QAbstractItemModel {
    Q_OBJECT

public:
    ModListItemModel(const QList<std::shared_ptr<ModListItem> >& mods, QObject* parent = nullptr)
        : QAbstractItemModel(parent), mMods(mods) {}

    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override
    {
        if (!hasIndex(row, column, parent)) {
            return QModelIndex();
        }
        return createIndex(row, column, mMods.at(row).get()); // store the raw pointer so we can cast later
    }

    QModelIndex parent(const QModelIndex&) const override
    {
        return QModelIndex();
    }

    int rowCount(const QModelIndex& parent = QModelIndex()) const override
    {
        return parent.isValid() ? 0 : mMods.size();
    }

    int columnCount(const QModelIndex& parent = QModelIndex()) const override
    {
        return parent.isValid() ? 0 : 2; // Mod name + patch count
    }

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override
    {
        if (!index.isValid() || role != Qt::DisplayRole) {
            return QVariant();
        }
        const auto mod = mMods.at(index.row());

        switch (index.column()) {
        case 0:
            if (mod && mod->modPtr) {
                return mod->modPtr->name();
            }
            break;
        case 1:
            return getMissingPluginCount(mod);
        default: ;
        }
        return "";
    }

    static int getMissingPluginCount(const std::shared_ptr<ModListItem>& mod)
    {
        int result = 0;
        for (auto val : mod->pluginToMentionsMap | std::views::values) {
            for (auto [_, fomodNotes] : val) {
                for (auto notInstalledPatchFor : fomodNotes.notInstalledPatchFor) {
                    if (std::ranges::find(mod->pluginNames, notInstalledPatchFor) != mod->pluginNames.end()) {
                        result += 1;
                    }
                }
            }
        }
        return result;
    }

    QVariant headerData(int column, Qt::Orientation orientation, int role = Qt::DisplayRole) const override
    {
        if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
            if (column == 0) {
                return QString("Mod");
            }
            return "Missing?";
        }
        return QVariant();
    }

private:
    QList<std::shared_ptr<ModListItem> > mMods;
};


#endif //MODLISTITEMMODEL_H