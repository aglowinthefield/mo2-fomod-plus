#include "FomodPlusPatchWizard.h"

#include <QDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

#include "lib/PatchFinder.h"

bool FomodPlusPatchWizard::init(IOrganizer* organizer)
{
    mOrganizer = organizer;
    mDialog = new QDialog();
    mDialog->setWindowTitle(tr("Patch Wizard"));
    mDialog->setMinimumSize(400, 200);
    log.setLogFilePath(QDir::currentPath().toStdString() + "/logs/fomodplus-patchwizard.log");

    mOrganizer->onUserInterfaceInitialized([this](QMainWindow*) {
        logMessage(DEBUG, "patches populated.");
        mPatchFinder = std::make_unique<PatchFinder>(mOrganizer);
        mPatchFinder->populateInstalledPlugins();
        mAvailablePatches = mPatchFinder->getAvailablePatchesForModList();
        logMessage(DEBUG, "Available Patches: " + std::to_string(mAvailablePatches.size()));
    });

    return true;
}

void FomodPlusPatchWizard::display() const
{
    // Clear any existing layout
    if (mDialog->layout() != nullptr) {
        QLayoutItem* item;
        while ((item = mDialog->layout()->takeAt(0)) != nullptr) {
            delete item->widget();
            delete item;
        }
        delete mDialog->layout();
    }

    if (mAvailablePatches.empty()) {
        setupEmptyState();
    } else {
        setupPatchList();
    }

    mDialog->exec();
}

void FomodPlusPatchWizard::setupEmptyState() const
{
    auto* mainLayout = new QVBoxLayout(mDialog);
    mainLayout->setAlignment(Qt::AlignCenter);

    auto* contentWidget = new QWidget(mDialog);
    auto* contentLayout = new QHBoxLayout(contentWidget);
    contentLayout->setAlignment(Qt::AlignCenter);
    contentLayout->setSpacing(16);

    auto* imageLabel = new QLabel(contentWidget);
    imageLabel->setPixmap(QPixmap(":/fomod/infoscroll").scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));

    auto* textLabel = new QLabel(
        tr("Nothing of interest yet. The wizard gets wiser as you\ninstall FOMODs, so check back later!"),
        contentWidget
    );
    textLabel->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);

    contentLayout->addWidget(imageLabel);
    contentLayout->addWidget(textLabel);

    mainLayout->addWidget(contentWidget);
}

void FomodPlusPatchWizard::setupPatchList() const
{
    auto* mainLayout = new QVBoxLayout(mDialog);

    auto* label = new QLabel(tr("Available patches: %1").arg(mAvailablePatches.size()), mDialog);
    mainLayout->addWidget(label);

    // TODO: Implement actual patch list UI
}