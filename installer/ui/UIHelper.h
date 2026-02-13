#pragma once

#include <QPushButton>
#include <QVBoxLayout>
#include <Qlabel>

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

class CtrlClickEventFilter final : public QObject {
    Q_OBJECT

  public:
    explicit CtrlClickEventFilter(const std::shared_ptr<PluginViewModel>& plugin,
        const std::shared_ptr<GroupViewModel>& group, QObject* parent = nullptr);

  signals:
    void ctrlClicked(
        bool selected, const std::shared_ptr<GroupViewModel>& group, const std::shared_ptr<PluginViewModel>& plugin);

  protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

  private:
    std::shared_ptr<PluginViewModel> mPlugin;
    std::shared_ptr<GroupViewModel> mGroup;
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