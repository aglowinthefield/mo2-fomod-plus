#include "FomodPlusPatchWizard.h"

#include <QApplication>
#include <QDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QProgressDialog>
#include <QPushButton>
#include <QVBoxLayout>

#include "lib/PatchFinder.h"
#include <FomodRescan.h>

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

    auto* rescanButton = new QPushButton(tr("Rescan Load Order"), mDialog);
    // Use const_cast since setupEmptyState is const but onRescanClicked modifies state
    connect(rescanButton, &QPushButton::clicked, const_cast<FomodPlusPatchWizard*>(this),
            &FomodPlusPatchWizard::onRescanClicked);
    mainLayout->addWidget(rescanButton, 0, Qt::AlignCenter);
}

void FomodPlusPatchWizard::onRescanClicked()
{
    const auto confirmResult = QMessageBox::question(
        mDialog,
        tr("Rescan Load Order"),
        tr("Rescanning will populate as many existing choices and options as we can, "
           "but if some downloads are deleted it may be missing things! "
           "It may take a few minutes depending on the size of your load order and all that."),
        QMessageBox::Ok | QMessageBox::Cancel,
        QMessageBox::Cancel
    );

    if (confirmResult != QMessageBox::Ok) {
        return;
    }

    logMessage(DEBUG, "Rescan requested by user");

    // Create progress dialog
    QProgressDialog progress(tr("Scanning mods..."), tr("Cancel"), 0, 100, mDialog);
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(0);
    progress.setValue(0);

    bool cancelled = false;

    // Perform the rescan
    FomodRescan rescan(mOrganizer, mPatchFinder->mFomodDb.get());
    auto result = rescan.scanAllModsWithChoices([&](int current, int total, const QString& modName) {
        if (progress.wasCanceled()) {
            cancelled = true;
            return;
        }
        const int percent = total > 0 ? (current * 100 / total) : 0;
        progress.setValue(percent);
        progress.setLabelText(tr("Scanning: %1 (%2/%3)").arg(modName).arg(current).arg(total));
        QApplication::processEvents();
    });

    progress.setValue(100);

    if (cancelled) {
        QMessageBox::information(
            mDialog,
            tr("Rescan Cancelled"),
            tr("The rescan was cancelled. Partial results may have been saved.")
        );
        logMessage(INFO, "Rescan cancelled by user");
    } else {
        // Show result summary
        QString summary = tr("Rescan complete!\n\n"
                            "Mods processed: %1\n"
                            "Successfully scanned: %2\n"
                            "Missing archives: %3\n"
                            "Parse errors: %4")
            .arg(result.totalModsProcessed)
            .arg(result.successfullyScanned)
            .arg(result.missingArchives)
            .arg(result.parseErrors);

        if (!result.failedMods.empty() && result.failedMods.size() <= 10) {
            summary += tr("\n\nFailed mods:");
            for (const auto& mod : result.failedMods) {
                summary += QString("\n- %1").arg(QString::fromStdString(mod));
            }
        } else if (result.failedMods.size() > 10) {
            summary += tr("\n\n%1 mods failed (see log for details)").arg(result.failedMods.size());
            for (const auto& mod : result.failedMods) {
                logMessage(INFO, "Failed mod: " + mod);
            }
        }

        QMessageBox::information(mDialog, tr("Rescan Complete"), summary);

        logMessage(INFO, "Rescan complete: " + std::to_string(result.successfullyScanned) +
                        "/" + std::to_string(result.totalModsProcessed) + " successful");
    }

    // Refresh available patches
    mPatchFinder->populateInstalledPlugins();
    mAvailablePatches = mPatchFinder->getAvailablePatchesForModList();
    logMessage(DEBUG, "Available Patches after rescan: " + std::to_string(mAvailablePatches.size()));

    // Refresh the UI
    display();
}

void FomodPlusPatchWizard::setupPatchList() const
{
    auto* mainLayout = new QVBoxLayout(mDialog);

    auto* label = new QLabel(tr("Available patches: %1").arg(mAvailablePatches.size()), mDialog);
    mainLayout->addWidget(label);

    // TODO: Implement actual patch list UI
}