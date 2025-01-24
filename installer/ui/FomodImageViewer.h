#ifndef FOMODIMAGEVIEWER_H
#define FOMODIMAGEVIEWER_H

#include "FomodViewModel.h"

#include <QDialog>
#include <QStackedWidget>

using LabelImagePair = std::pair<QString, QString>;


/*
+----------------------------------------------------------+
|n/N                                                    X  |
+---+--------------------------------------------------+---+
|   |                                                  |   |
|   |                                                  |   |
|   |                                                  |   |
|   |                                                  |   |
|   |                                                  |   |
|   |                                                  |   |
| < |               Image                              | > |
|   |                                                  |   |
|   |                                                  |   |
|   |                                                  |   |
|   |                                                  |   |
|   |                 label                            |   |
+------+------+------+---------------------------------+---+
|      |      |      |  ...previews                        |
|      |      |      |                                     |
+------+------+------+-------------------------------------+
*/

class FomodImageViewer final : public QDialog {
    Q_OBJECT

public:
    explicit FomodImageViewer(QWidget* parent,
        const QString& fomodPath,
        const std::shared_ptr<StepViewModel>& activeStep,
        const std::shared_ptr<PluginViewModel>& activePlugin);

private:
    void collectImages();

    QWidget* createCenterRow(QWidget* parent);

    QStackedWidget* createStackWidget(QWidget *parent);

    QWidget* createSinglePhotoPane(QWidget* parent, const QString& imagePath);

    QWidget* createPreviewImages(QWidget *parent) const;

    QWidget* createBackButton(QWidget *parent);

    QWidget* createForwardButton(QWidget *parent);

    QWidget* createMainImageLabel(QWidget *parent);

    QWidget* createTopBar(QWidget *parent);

    QWidget* createCloseButton(QWidget* parent);

    QWidget* createCounter(QWidget *parent) const;

    void goBack();

    void goForward();

    std::vector<LabelImagePair> mLabelsAndImages;
    int mCurrentIndex{ 0 };

    QString mFomodPath;
    const std::shared_ptr<StepViewModel>& mActiveStep;
    const std::shared_ptr<PluginViewModel>& mActivePlugin;

    QWidget* mCenterRow{ nullptr };
    QWidget* mTopBar{ nullptr };
    QWidget* mCloseButton{ nullptr };
    QWidget* mBackButton{ nullptr };
    QWidget* mForwardButton{ nullptr };
    QWidget* mCounter{ nullptr };
    QStackedWidget* mMainDisplayImage{ nullptr };
    QWidget* mLabel{ nullptr };
    QWidget* mPreviewImages{ nullptr };
};


#endif //FOMODIMAGEVIEWER_H