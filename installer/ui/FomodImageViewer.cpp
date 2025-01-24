#include "FomodImageViewer.h"

#include "ScaleLabel.h"
#include "UIHelper.h"

#include <QStackedWidget>
#include <QLabel>
#include <QScrollArea>

constexpr int PREVIEW_IMAGE_HEIGHT = 100;
constexpr int PREVIEW_IMAGE_WIDTH  = 120;

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
FomodImageViewer::FomodImageViewer(QWidget* parent,
    const QString& fomodPath,
    const std::shared_ptr<StepViewModel>& activeStep,
    const std::shared_ptr<PluginViewModel>& activePlugin) : QDialog(parent), mFomodPath(fomodPath),
                                                            mActiveStep(activeStep),
                                                            mActivePlugin(activePlugin)
{
    collectImages();
    mMainDisplayImage = createStackWidget(this);
    mTopBar           = createTopBar(this);
    mPreviewImages    = createPreviewImages(this);
    mCenterRow        = createCenterRow(this);

    const auto layout = new QVBoxLayout(this);
    layout->addWidget(mTopBar);
    layout->addWidget(mCenterRow, 1);
    layout->addWidget(mPreviewImages);
    setLayout(layout);
}

void FomodImageViewer::collectImages()
{
    mLabelsAndImages.clear();
    for (const auto& groupViewModel : mActiveStep->getGroups()) {
        for (const auto& pluginViewModel : groupViewModel->getPlugins()) {
            QString imagePath = UIHelper::getFullImagePath(mFomodPath,
                QString::fromStdString(pluginViewModel->getImagePath()));
            mLabelsAndImages.emplace_back(QString::fromStdString(pluginViewModel->getName()), imagePath);

            if (pluginViewModel == mActivePlugin) {
                mCurrentIndex = static_cast<int>(mLabelsAndImages.size()) - 1;
            }
        }
    }
}

QWidget* FomodImageViewer::createCenterRow(QWidget* parent)
{
    const auto centerRow = new QWidget(parent);
    const auto layout    = new QHBoxLayout(centerRow);

    mBackButton    = createBackButton(centerRow);
    mForwardButton = createForwardButton(centerRow);

    layout->addWidget(mBackButton);
    layout->addWidget(mMainDisplayImage, 1);
    layout->addWidget(mForwardButton);

    return centerRow;
}

QStackedWidget* FomodImageViewer::createStackWidget(QWidget* parent)
{
    const auto stackWidget = new QStackedWidget(parent);
    for (const auto& val : mLabelsAndImages | std::views::values) {
        const auto photoPane = createSinglePhotoPane(stackWidget, val);
        stackWidget->addWidget(photoPane);
    }
    return stackWidget;
}

QWidget* FomodImageViewer::createSinglePhotoPane(QWidget* parent, const QString& imagePath)
{
    const auto singlePhotoPane = new QWidget(parent);
    const auto layout          = new QVBoxLayout(singlePhotoPane);

    const auto imageLabel = new ScaleLabel(singlePhotoPane);
    imageLabel->setScalableResource(imagePath);
    layout->addWidget(imageLabel);

    return singlePhotoPane;
}

QWidget* FomodImageViewer::createPreviewImages(QWidget* parent) const
{
    const auto previewImages = new QScrollArea(parent);
    const auto widget        = new QWidget(previewImages);
    const auto layout        = new QHBoxLayout(previewImages);

    for (const auto& val : mLabelsAndImages | std::views::values) {
        const auto imageLabel = new ScaleLabel(previewImages);
        imageLabel->setScalableResource(val);
        layout->addWidget(imageLabel);
    }

    widget->setLayout(layout);
    previewImages->setWidget(widget);

    return previewImages;
}

QWidget* FomodImageViewer::createBackButton(QWidget* parent)
{
    const auto backButton = new QPushButton(parent);
    connect(backButton, &QPushButton::clicked, this, &FomodImageViewer::goBack);
    return backButton;
}

QWidget* FomodImageViewer::createForwardButton(QWidget* parent)
{
    const auto forwardButton = new QPushButton(parent);
    connect(forwardButton, &QPushButton::clicked, this, &FomodImageViewer::goForward);
    return forwardButton;
}

QWidget* FomodImageViewer::createMainImageLabel(QWidget* parent)
{
    const auto forwardButton = new QWidget(parent);
    return forwardButton;
}

QWidget* FomodImageViewer::createTopBar(QWidget* parent)
{
    const auto topBar = new QWidget(parent);
    const auto layout = new QHBoxLayout(topBar);

    // counter, spacer, close button
    mCounter = createCounter(topBar);
    layout->addWidget(mCounter);

    layout->addStretch();

    mCloseButton = createCloseButton(topBar);
    layout->addWidget(mCloseButton);
    return topBar;
}

QWidget* FomodImageViewer::createCloseButton(QWidget* parent)
{
    const auto closeButton = new QPushButton(parent);
    connect(closeButton, &QPushButton::clicked, this, &FomodImageViewer::close);
    return closeButton;
}

QWidget* FomodImageViewer::createCounter(QWidget* parent) const
{
    const auto counter = new QLabel(parent);
    counter->setText(QString::number(mCurrentIndex + 1) + "/" + QString::number(mLabelsAndImages.size()));
    return counter;
}

void FomodImageViewer::goBack()
{
    if (mCurrentIndex == 0) {
        return;
    }
    mMainDisplayImage->setCurrentIndex(--mCurrentIndex);
}

void FomodImageViewer::goForward()
{
    if (mCurrentIndex == mLabelsAndImages.size() - 1) {
        return;
    }
    mMainDisplayImage->setCurrentIndex(++mCurrentIndex);
}