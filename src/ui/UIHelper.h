#ifndef UIHELPER_H
#define UIHELPER_H


#include <QPushButton>
#include <Qlabel>

class UIHelper {
public:
  static QPushButton* createButton(const QString& text, QWidget* parent = nullptr) {
    QPushButton* button = new QPushButton(text, parent);
    return button;
  }

  static QLabel* createLabel(const QString& text, QWidget* parent = nullptr) {
    QLabel* label = new QLabel(text, parent);
    return label;
  }

};

#endif //UIHELPER_H
