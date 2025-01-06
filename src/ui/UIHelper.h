#ifndef UIHELPER_H
#define UIHELPER_H


#include <QPushButton>
#include <Qlabel>
#include <QVBoxLayout>

namespace UiConstants {
  constexpr int WINDOW_MIN_WIDTH = 1000;
  constexpr int WINDOW_MIN_HEIGHT = 700;
}

class UIHelper {
public:
  static QPushButton *createButton(const QString &text, QWidget *parent);
  static QLabel *createLabel(const QString &text, QWidget *parent);
  static QLabel *createHyperlink(const QString &url, QWidget *parent);

  static QString getFullImagePath(const QString& fomodPath, const QString &imagePath);

  static void setGlobalAlignment(QBoxLayout *layout, Qt::Alignment alignment);
  static void setDebugBorders(QWidget *widget);
  static void reduceLabelPadding(const QLayout *layout);
};

#endif //UIHELPER_H
