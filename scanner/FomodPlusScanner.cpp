#include "FomodPlusScanner.h"

#include <QDialog>
#include <QPushButton>
#include <QVBoxLayout>

bool FomodPlusScanner::init(IOrganizer* organizer) {
  mOrganizer = organizer;
  return true;
}

void FomodPlusScanner::display() const {
  QDialog dialog;
  dialog.setWindowTitle("FOMOD Scanner");

  QVBoxLayout layout(&dialog);

  QPushButton scanButton("Scan", &dialog);
  QPushButton cancelButton("Cancel", &dialog);

  layout.addWidget(&scanButton);
  layout.addWidget(&cancelButton);

  connect(&cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);
  connect(&scanButton, &QPushButton::clicked, [&dialog] {
      dialog.accept();
  });

  dialog.exec();

}