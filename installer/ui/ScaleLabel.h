#ifndef SCALELABEL_H
#define SCALELABEL_H

// Taken from https://github.com/ModOrganizer2/modorganizer-installer_fomod/blob/master/src/scalelabel.h
#include <QLabel>
#include <QMovie>


class ScaleLabel final : public QLabel {
  Q_OBJECT

public:
  explicit ScaleLabel(QWidget *parent = nullptr);

  void setScalableResource(const QString &path);

  void setStatic(bool isStatic);

protected:
  void resizeEvent(QResizeEvent *event) override;

private:
  void setScalableMovie(const QString &path);

  void setScalableImage(const QString &path);

  QImage mUnscaledImage;
  QSize mOriginalMovieSize;
  bool misStatic = false;
};


#endif //SCALELABEL_H
