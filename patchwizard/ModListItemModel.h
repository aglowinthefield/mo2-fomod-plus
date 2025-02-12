#ifndef MODLISTITEMMODEL_H
#define MODLISTITEMMODEL_H


#include <QAbstractItemModel>
#include <imodinterface.h>

struct ModListItem {
    MOBase::IModInterface* modPtr;
    std::vector<QString> pluginNames;
};

class ModListItemModel: public QAbstractItemModel
{
    Q_OBJECT
public:
    ModListItemModel(const QList<std::shared_ptr<ModListItem>>& mods, QObject* parent = nullptr)
        : QAbstractItemModel(parent), mMods(mods) {}

    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override {
        if (!hasIndex(row, column, parent)) {
            return QModelIndex();
        }
        return createIndex(row, column, mMods.at(row).get()); // store the raw pointer so we can cast later
    }

    QModelIndex parent(const QModelIndex&) const override {
        return QModelIndex();
    }

    int rowCount(const QModelIndex& parent = QModelIndex()) const override {
        return parent.isValid() ? 0 : mMods.size();
    }

    int columnCount(const QModelIndex& parent = QModelIndex()) const override {
        return parent.isValid() ? 0 : 2; // Mod name + patch count
    }

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override {
        if (!index.isValid() || role != Qt::DisplayRole) {
            return QVariant();
        }
        auto mod = mMods.at(index.row());

        if (index.column() == 0) {
            if (mod && mod->modPtr) {
                return mod->modPtr->name();
            }
        }
        return 0;
    }

    QVariant headerData(int column, Qt::Orientation orientation, int role = Qt::DisplayRole) const override {
        if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
            if (column == 0) {
                return QString("Mod");
            }
            return "Patches";
        }
        return QVariant();
    }

private:
    QList<std::shared_ptr<ModListItem>> mMods;
};



#endif //MODLISTITEMMODEL_H
