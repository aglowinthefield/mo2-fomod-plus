#include "UIHelper.h"

#include <qcoreevent.h>
#include <QDir>

HoverEventFilter::HoverEventFilter(const std::shared_ptr<PluginViewModel>& plugin, QObject* parent)
    : QObject(parent), mPlugin(plugin) {}

bool HoverEventFilter::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() == QEvent::HoverEnter) {
        emit hovered(mPlugin);
        return true;
    }
    return QObject::eventFilter(obj, event);
}

QPushButton* UIHelper::createButton(const QString& text, QWidget* parent = nullptr)
{
    const auto button = new QPushButton(text, parent);
    return button;
}

QLabel* UIHelper::createLabel(const QString& text, QWidget* parent = nullptr)
{
    const auto label = new QLabel(text, parent);
    return label;
}

QLabel* UIHelper::createHyperlink(const QString& url, QWidget* parent = nullptr)
{
    if (url.isEmpty() || !QUrl(url).isValid()) {
        return createLabel(url, parent);
    }
    const auto label        = new QLabel(url, parent);
    const QString hyperlink = QString("<a href=\"%1\">%1</a>").arg(url);
    label->setText(hyperlink);
    label->setOpenExternalLinks(true);
    label->setTextFormat(Qt::RichText);
    return label;
}

QString UIHelper::getFullImagePath(const QString& fomodPath, const QString& imagePath)
{
    return QDir::tempPath() + "/" + fomodPath + "/" + imagePath;
}

void UIHelper::setGlobalAlignment(QBoxLayout* layout, const Qt::Alignment alignment)
{
    for (int i = 0; i < layout->count(); ++i) {
        if (const QLayoutItem* item = layout->itemAt(i); item->widget()) {
            layout->setAlignment(item->widget(), alignment);
        }
    }
}

void UIHelper::setDebugBorders(QWidget* widget)
{
    widget->setStyleSheet("border: 1px solid red;");
    for (auto* child : widget->findChildren<QWidget*>()) {
        child->setStyleSheet("border: 1px solid red;");
    }
}

void UIHelper::reduceLabelPadding(const QLayout* layout)
{
    for (int i = 0; i < layout->count(); ++i) {
        const QLayoutItem* item = layout->itemAt(i);
        if (QWidget* widget = item->widget()) {
            if (const auto label = qobject_cast<QLabel*>(widget)) {
                label->setContentsMargins(0, 0, 0, 0);
                label->setStyleSheet("padding: 0px; margin: 0px;");
            }
        }
    }
}