#include "ModDetailsWidget.h"

#include <QLabel>
#include <QScrollArea>
#include <QVBoxLayout>

ModDetailsWidget::ModDetailsWidget(const ModListItem* modItem, QWidget*)
{
    const auto layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);

    const auto scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);

    const auto wrapper       = new QWidget(scrollArea);
    const auto wrapperLayout = new QVBoxLayout(wrapper);

    // Add mod name
    wrapperLayout->addWidget(new QLabel(modItem->modPtr->name()));

    // Add more content as needed
    // Example: Add a list of mentions
    QString mentions;
    for (const auto& [plugin, mods] : modItem->pluginToMentionsMap) {
        mentions += plugin + ":\n";
        for (const auto& mod : mods) {
            mentions += "\t- " + mod->name() + "\n";
        }
    }
    wrapperLayout->addWidget(new QLabel(mentions));

    scrollArea->setWidget(wrapper);
    layout->addWidget(scrollArea);
}