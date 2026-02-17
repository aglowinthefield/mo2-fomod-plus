#include "UIHelper.h"

#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QDir>
#include <QMenu>
#include <QMouseEvent>
#include <QUrl>
#include <qcoreevent.h>

HoverEventFilter::HoverEventFilter(const std::shared_ptr<PluginViewModel>& plugin, QObject* parent)
    : QObject(parent)
    , mPlugin(plugin)
{
}

bool HoverEventFilter::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() == QEvent::HoverEnter) {
        emit hovered(mPlugin);
        return true;
    }
    return QObject::eventFilter(obj, event);
}

CtrlClickEventFilter::CtrlClickEventFilter(
    const std::shared_ptr<PluginViewModel>& plugin, const std::shared_ptr<GroupViewModel>& group, QObject* parent)
    : QObject(parent)
    , mPlugin(plugin)
    , mGroup(group)
{
}

bool CtrlClickEventFilter::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        const QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton && mouseEvent->modifiers() & Qt::ControlModifier) {
            // TODO: Add Ctrl+click handler logic here
            std::cout << "Ctrl+click detected on plugin: " << mPlugin->getName() << " in group: " << mGroup->getName()
                      << std::endl;
            // For now, just fall through to default behavior
        }
    }
    return QObject::eventFilter(obj, event);
}

ContextMenuEventFilter::ContextMenuEventFilter(const std::shared_ptr<PluginViewModel>& plugin,
    const std::shared_ptr<GroupViewModel>& group, const std::shared_ptr<StepViewModel>& step,
    const QString& nexusGameName, QObject* parent)
    : QObject(parent)
    , mPlugin(plugin)
    , mGroup(group)
    , mStep(step)
    , mNexusGameName(nexusGameName)
{
}

bool ContextMenuEventFilter::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() == QEvent::ContextMenu) {
        auto* widget = qobject_cast<QWidget*>(obj);
        if (!widget) {
            return QObject::eventFilter(obj, event);
        }

        QMenu menu(widget);

        const auto pluginName = QString::fromStdString(mPlugin->getName());
        const auto description = QString::fromStdString(mPlugin->getDescription());
        const auto groupName = QString::fromStdString(mGroup->getName());
        const auto stepName = QString::fromStdString(mStep->getName());

        menu.addAction("Copy Option Name", [pluginName]() {
            QApplication::clipboard()->setText(pluginName);
        });

        menu.addAction("Copy Description", [description]() {
            QApplication::clipboard()->setText(description);
        });

        menu.addAction("Copy Group Name", [groupName]() {
            QApplication::clipboard()->setText(groupName);
        });

        menu.addAction("Copy Step Name", [stepName]() {
            QApplication::clipboard()->setText(stepName);
        });

        if (!mNexusGameName.isEmpty()) {
            menu.addSeparator();
            menu.addAction("Search on Nexus", [this, pluginName]() {
                auto keyword = QString::fromUtf8(QUrl::toPercentEncoding(pluginName));
                keyword.replace("%20", "+");
                const auto url = QString("https://www.nexusmods.com/games/%1/mods?keyword=%2&sort=endorsements")
                                     .arg(mNexusGameName, keyword);
                Logger::getInstance().logMessage(INFO, "[ContextMenu] Opening Nexus URL: " + url.toStdString());
                QDesktopServices::openUrl(QUrl(url));
            });
        }

        menu.exec(QCursor::pos());
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
    const QString hyperlink = QString("<a href=\"%1\">%2</a>").arg(url, "Link");
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
