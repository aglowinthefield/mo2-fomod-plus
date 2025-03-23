#include "FomodImageViewer.h"

#include "ScaleLabel.h"
#include "UIHelper.h"

#include <QLabel>
#include <QScrollArea>
#include <QtConcurrent/QtConcurrent>

constexpr int PREVIEW_IMAGE_WIDTH  = 160;
constexpr int PREVIEW_IMAGE_HEIGHT = 90;

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
constexpr auto BUTTON_STYLE =
    "font-size: 16px; font-weight: bold; color: white; background-color: black; padding: 5px; border-radius: 1px solid black;";

FomodImageViewer::FomodImageViewer(QWidget* parent,
    const QString& fomodPath,
    const std::shared_ptr<StepViewModel>& activeStep,
    const std::shared_ptr<PluginViewModel>& activePlugin) : QDialog(parent), mFomodPath(fomodPath),
                                                            mActiveStep(activeStep),
                                                            mActivePlugin(activePlugin)
{

    setWindowFlags(Qt::FramelessWindowHint | Qt::Window);
    setAttribute(Qt::WA_TranslucentBackground);
    setStyleSheet("background-color: rgba(0, 0, 0, 150);");

    const QScreen* screen         = this->screen();
    const QRect availableGeometry = screen->availableGeometry();
    setFixedSize(availableGeometry.width(), availableGeometry.height());
    move(availableGeometry.x(), availableGeometry.y());

    collectImages();
    mMainImageWrapper = createSinglePhotoPane(this);
    mTopBar           = createTopBar(this);
    mPreviewImages    = createPreviewImages(this);
    mCenterRow        = createCenterRow(this);

    const auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0); // Remove margins
    layout->setSpacing(0); // Remove spacing between widgets
    layout->addWidget(mTopBar);
    layout->addWidget(mCenterRow, 1);
    layout->addWidget(mPreviewImages);
    setLayout(layout);

    select(mCurrentIndex);

    setFocusPolicy(Qt::StrongFocus); // so we can receive key events
}

void FomodImageViewer::collectImages()
{
    mLabelsAndImages.clear();
    for (const auto& groupViewModel : mActiveStep->getGroups()) {
        for (const auto& pluginViewModel : groupViewModel->getPlugins()) {
            if (pluginViewModel->getImagePath().empty()) {
                continue;
            }
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

    mBackButton->setFocusPolicy(Qt::NoFocus);
    mForwardButton->setFocusPolicy(Qt::NoFocus);

    layout->addWidget(mBackButton);
    layout->addWidget(mMainImageWrapper, 1);
    layout->addWidget(mForwardButton);

    return centerRow;
}

// ReSharper disable once CppMemberFunctionMayBeStatic
QWidget* FomodImageViewer::createSinglePhotoPane(QWidget* parent)
{
    const auto singlePhotoPane = new QWidget(parent);
    const auto layout          = new QVBoxLayout(singlePhotoPane);

    // const auto [labelText, imagePath] = pair;

    mMainImage = new ScaleLabel(singlePhotoPane);
    mMainImage->setAlignment(Qt::AlignCenter);
    layout->addWidget(mMainImage, 1);

    mLabel = new QLabel(singlePhotoPane);
    // mLabel->setText(labelText);
    mLabel->setAlignment(Qt::AlignCenter);
    mLabel->setStyleSheet("color: white; font-size: 20px;");
    layout->addWidget(mLabel);

    return singlePhotoPane;
}

QScrollArea* FomodImageViewer::createPreviewImages(QWidget* parent)
{
    mImagePanes.clear();
    const auto previewImages = new QScrollArea(parent);
    const auto widget        = new QWidget(previewImages);
    const auto layout        = new QHBoxLayout(previewImages);

    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    for (int i = 0; i < mLabelsAndImages.size(); i++) {
        const auto imageLabel = new ScaleLabel(previewImages);
        imageLabel->setAlignment(Qt::AlignCenter);
        imageLabel->setFixedSize(PREVIEW_IMAGE_WIDTH, PREVIEW_IMAGE_HEIGHT);
        imageLabel->setFocusPolicy(Qt::NoFocus);
        connect(imageLabel, &ScaleLabel::clicked, this, [this, i] {
            select(i);
        });
        layout->addWidget(imageLabel);
        mImagePanes.emplace_back(imageLabel);

        // imageLabel->setScalableResource(mLabelsAndImages[i].second);
        const auto imagePath = mLabelsAndImages[i].second;

        QThreadPool::globalInstance()->start([imageLabel, imagePath]() {
            QMetaObject::invokeMethod(imageLabel, [imageLabel, imagePath]() {
                imageLabel->setScalableResource(imagePath);
            }, Qt::QueuedConnection);
        });
    }

    widget->setLayout(layout);
    previewImages->setFixedHeight(PREVIEW_IMAGE_HEIGHT + 10);
    previewImages->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    previewImages->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    previewImages->setFocusPolicy(Qt::NoFocus);

    previewImages->setWidget(widget);
    previewImages->setStyleSheet("QScrollArea { border: none; }");

    return previewImages;
}

QPushButton* FomodImageViewer::createBackButton(QWidget* parent) const
{
    const auto backButton = new QPushButton(parent);
    backButton->setText("<");
    backButton->setStyleSheet(BUTTON_STYLE);
    connect(backButton, &QPushButton::clicked, this, &FomodImageViewer::goBack);
    return backButton;
}

QPushButton* FomodImageViewer::createForwardButton(QWidget* parent) const
{
    const auto forwardButton = new QPushButton(parent);
    forwardButton->setText(">");
    forwardButton->
        setStyleSheet(BUTTON_STYLE);
    connect(forwardButton, &QPushButton::clicked, this, &FomodImageViewer::goForward);
    return forwardButton;
}

QWidget* FomodImageViewer::createTopBar(QWidget* parent)
{
    const auto topBar = new QWidget(parent);
    const auto layout = new QHBoxLayout(topBar);

    // counter, spacer, close button
    mCounter = new QLabel(topBar);
    mCounter->setStyleSheet(BUTTON_STYLE);
    layout->addWidget(mCounter);

    layout->addStretch();

    mCloseButton = createCloseButton(topBar);
    layout->addWidget(mCloseButton);
    return topBar;
}

QPushButton* FomodImageViewer::createCloseButton(QWidget* parent)
{
    const auto closeButton = new QPushButton(parent);
    // const QIcon icon(":/fomod/close");
    // closeButton->setIcon(icon);
    closeButton->setText("X");
    closeButton->setStyleSheet("color: white; background-color: black; padding: 5px; border-radius: 1px solid black;");
    connect(closeButton, &QPushButton::clicked, this, &FomodImageViewer::close);
    return closeButton;
}

void FomodImageViewer::updateCounterText() const
{
    mCounter->setText(QString::number(mCurrentIndex + 1) + "/" + QString::number(mLabelsAndImages.size()));
}

void FomodImageViewer::goBack()
{
    if (mCurrentIndex == 0) {
        return;
    }
    select(--mCurrentIndex);
}

void FomodImageViewer::goForward()
{
    if (mCurrentIndex == mLabelsAndImages.size() - 1) {
        return;
    }
    select(++mCurrentIndex);
}

void FomodImageViewer::select(const int index)
{
    if (index < 0 || index >= mLabelsAndImages.size()) {
        return;
    }

    // Remove border from previously selected image
    mCurrentIndex = index; // check for bounds?
    updateCounterText();

    for (int i = 0; i < mImagePanes.size(); i++) {
        if (i == mCurrentIndex) {
            mImagePanes[i]->setStyleSheet("border: 2px solid white;");
        } else {
            mImagePanes[i]->setStyleSheet("");
        }
    }

    const auto& imagePath = mLabelsAndImages[index].second;
    const auto& labelText = mLabelsAndImages[index].first;
    mLabel->setText(labelText);
    mMainImage->setScalableResource(imagePath);
}

void FomodImageViewer::keyPressEvent(QKeyEvent* event)
{
    switch (event->key()) {
    case Qt::Key_Left:
        goBack();
        break;
    case Qt::Key_Right:
        goForward();
        break;
    default:
        QDialog::keyPressEvent(event);
    }
}

void FomodImageViewer::showEvent(QShowEvent* event)
{
    QDialog::showEvent(event);
    setFocus();
}