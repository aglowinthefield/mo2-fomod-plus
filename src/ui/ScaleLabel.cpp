#include "scalelabel.h"
#include <QResizeEvent>

// Taken from https://github.com/ModOrganizer2/modorganizer-installer_fomod/blob/master/src/scalelabel.h
static bool isResourceMovie(const QString& path)
{
  const auto formats = QMovie::supportedFormats();
  return std::ranges::any_of(formats, [&path](const QByteArray& format) {
    return path.endsWith("." + QString::fromUtf8(format));
  });
}

ScaleLabel::ScaleLabel(QWidget* parent) : QLabel(parent) {}

void ScaleLabel::setScalableResource(const QString& path)
{
  if (const auto m = movie()) {
    setMovie(nullptr);
    delete m;
    mOriginalMovieSize = QSize();
  }
  if (!pixmap().isNull()) {
    setPixmap(QPixmap());
    mUnscaledImage = QImage();
  }

  if (path.isEmpty()) {
    return;
  }

  if (isResourceMovie(path)) {
    setScalableMovie(path);
  } else {
    setScalableImage(path);
  }
}

void ScaleLabel::setStatic(const bool isStatic)
{
  misStatic = isStatic;

  if (const auto m = movie()) {
    if (isStatic) {
      m->stop();
    } else {
      m->start();
    }
  }
}

void ScaleLabel::setScalableMovie(const QString& path)
{
  const auto m = new QMovie(path);
  if (!m->isValid()) {
    qWarning(">%s< is an invalid movie. Reason: %s", qUtf8Printable(path),
             m->lastErrorString().toStdString().c_str());
    delete m;
    return;
  }

  m->setParent(this);
  setMovie(m);
  m->start();
  m->stop();
  mOriginalMovieSize = m->currentImage().size();

  m->setScaledSize(mOriginalMovieSize.scaled(size(), Qt::KeepAspectRatio));
  if (!misStatic) {
    m->start();
  }
}

void ScaleLabel::setScalableImage(const QString& path)
{
  if (const QImage image(path); image.isNull()) {
    qWarning(">%s< is a null image", qUtf8Printable(path));
  } else {
    mUnscaledImage = image;
    setPixmap(QPixmap::fromImage(image).scaled(size(), Qt::KeepAspectRatio));
  }
}

void ScaleLabel::resizeEvent(QResizeEvent* event)
{
  if (const auto m = movie()) {
    m->stop();
    m->setScaledSize(mOriginalMovieSize.scaled(event->size(), Qt::KeepAspectRatio));
    m->start();

    // We can't just skip the start() above since that is what triggers the label to
    // resize the movie The only way to resize the movie but keep it paused is to start
    // and then re-stop it
    if (misStatic) {
      m->stop();
    }
  }
  if (const auto p = pixmap(); !p.isNull()) {
    setPixmap(
        QPixmap::fromImage(mUnscaledImage).scaled(event->size(), Qt::KeepAspectRatio));
  }
}
