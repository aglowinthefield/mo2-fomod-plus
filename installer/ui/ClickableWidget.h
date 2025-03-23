#pragma once

#include <QMouseEvent>
#include <QWidget>

class ClickableWidget final : public QWidget {
    Q_OBJECT

public:
    explicit ClickableWidget(QWidget* parent = nullptr) : QWidget(parent) {
        setCursor(Qt::PointingHandCursor);
    }

    signals:
        void clicked();

protected:
    void mousePressEvent(QMouseEvent* event) override {
        if (event->button() == Qt::LeftButton) {
            emit clicked();
        }
        QWidget::mousePressEvent(event);
    }
};
