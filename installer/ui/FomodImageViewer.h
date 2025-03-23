#pragma once

#include "FomodViewModel.h"

#include <QDialog>
#include <QKeyEvent>
#include <QLabel>
#include <QScrollArea>

class ScaleLabel;
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

    QWidget* createSinglePhotoPane(QWidget* parent);

    QScrollArea* createPreviewImages(QWidget* parent);

    QPushButton* createBackButton(QWidget* parent) const;

    QPushButton* createForwardButton(QWidget* parent) const;

    QWidget* createTopBar(QWidget* parent);

    QPushButton* createCloseButton(QWidget* parent);

    void updateCounterText() const;

    void goBack();

    void goForward();

    void select(int index);

    void keyPressEvent(QKeyEvent* event) override;

    void showEvent(QShowEvent* event) override;

    std::vector<LabelImagePair> mLabelsAndImages;
    std::vector<QWidget*> mImagePanes{};
    int mCurrentIndex{ 0 };

    QString mFomodPath;
    const std::shared_ptr<StepViewModel>& mActiveStep;
    const std::shared_ptr<PluginViewModel>& mActivePlugin;

    QWidget* mCenterRow{ nullptr };
    QWidget* mTopBar{ nullptr };
    QWidget* mCloseButton{ nullptr };
    QPushButton* mBackButton{ nullptr };
    QPushButton* mForwardButton{ nullptr };
    QLabel* mCounter{ nullptr };
    QWidget* mMainImageWrapper{ nullptr };
    ScaleLabel* mMainImage{ nullptr };
    QLabel* mLabel{ nullptr };
    QScrollArea* mPreviewImages{ nullptr };
};