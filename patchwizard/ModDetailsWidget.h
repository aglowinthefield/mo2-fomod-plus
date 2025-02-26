#ifndef MODDETAILSWIDGET_H
#define MODDETAILSWIDGET_H

#include "ModListItemModel.h"

#include <QWidget>
#include <qtmetamacros.h>

class ModDetailsWidget final : public QWidget {
    Q_OBJECT

public:
    explicit ModDetailsWidget(const ModListItem* modItem, QWidget* parent = nullptr);
};



#endif //MODDETAILSWIDGET_H
