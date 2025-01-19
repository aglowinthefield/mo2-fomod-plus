#ifndef UIHELPER_H
#define UIHELPER_H


#include <QPushButton>
#include <Qlabel>
#include <QVBoxLayout>

#include "FomodViewModel.h"

class HoverEventFilter final : public QObject {
    Q_OBJECT

public:
    explicit HoverEventFilter(const std::shared_ptr<PluginViewModel>& plugin, QObject* parent = nullptr);

signals:
    void hovered(const std::shared_ptr<PluginViewModel>& plugin);

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    std::shared_ptr<PluginViewModel> mPlugin;
};


namespace UiConstants {
constexpr int WINDOW_MIN_WIDTH  = 900;
constexpr int WINDOW_MIN_HEIGHT = 600;
}

class UIHelper {
public:
    /*
    --------------------------------------------------------------------------------
                               Widgets & Events
    --------------------------------------------------------------------------------
    */
    static QPushButton* createButton(const QString& text, QWidget* parent);

    static QLabel* createLabel(const QString& text, QWidget* parent);

    static QLabel* createHyperlink(const QString& url, QWidget* parent);

    /*
    --------------------------------------------------------------------------------
                                   Helpers
    --------------------------------------------------------------------------------
    */
    static QString getFullImagePath(const QString& fomodPath, const QString& imagePath);

    static void setGlobalAlignment(QBoxLayout* layout, Qt::Alignment alignment);

    static void reduceLabelPadding(const QLayout* layout);

    /*
    --------------------------------------------------------------------------------
                                   Development
    --------------------------------------------------------------------------------
    */
    static void setDebugBorders(QWidget* widget);
};

#endif //UIHELPER_H