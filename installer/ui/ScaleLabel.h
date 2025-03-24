#pragma once

// Taken from https://github.com/ModOrganizer2/modorganizer-installer_fomod/blob/master/src/scalelabel.h
#include <QLabel>
#include <QMouseEvent>
#include <QMovie>


class ScaleLabel final : public QLabel {
    Q_OBJECT

public:
    explicit ScaleLabel(QWidget* parent = nullptr) : QLabel(parent)
    {
        setCursor(Qt::PointingHandCursor);
    }

    void setScalableResource(const QString& path);
    void setStatic(bool isStatic);
    [[nodiscard]] bool hasResource() const { return mHasResource; }

signals:
    void clicked();

protected:
    void mousePressEvent(QMouseEvent* event) override
    {
        if (event->button() == Qt::LeftButton) {
            emit clicked();
        }
        QLabel::mousePressEvent(event);
    }

    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;

private:
    void setScalableMovie(const QString& path);
    void setScalableImage(const QString& path);

    QImage mUnscaledImage;
    QSize mOriginalMovieSize;
    bool mHasResource = false;
    bool misStatic    = false;
};