#include "ModDetailsWidget.h"

#include <QHeaderView>
#include <QLabel>
#include <QVBoxLayout>
#include <QTableWidgetItem>

ModDetailsWidget::ModDetailsWidget(const ModListItem* modItem, QWidget*)
{
    const auto layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);

    const auto tableWidget = new QTableWidget(this);
    tableWidget->setColumnCount(3);
    tableWidget->setHorizontalHeaderLabels({ tr("FOMOD"), tr("Has Patch For"), tr("Install State") });
    tableWidget->setSortingEnabled(true);

    int row = 0;
    for (const auto& [plugin, mods] : modItem->pluginToMentionsMap) {
        for (const auto& [modPtr, fomodNotes] : mods) {
            tableWidget->insertRow(row);

            // Mod name
            const auto modNameItem = new QTableWidgetItem(modPtr->name());
            tableWidget->setItem(row, 0, modNameItem);

            // Has patch for (should be plugin name)
            const auto hasPatchForItem = new QTableWidgetItem(plugin);
            tableWidget->setItem(row, 1, hasPatchForItem);

            // Install state
            QString installState;
            if (fomodNotes.installedPatchFor.contains(plugin)) {
                installState = tr("Installed");
            } else if (fomodNotes.notInstalledPatchFor.contains(plugin)) {
                installState = tr("Not Installed");
            } else {
                installState = tr("Unknown");
            }
            const auto installStateItem = new QTableWidgetItem(installState);
            tableWidget->setItem(row, 2, installStateItem);

            ++row;
        }
    }

    tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    layout->addWidget(tableWidget);
}