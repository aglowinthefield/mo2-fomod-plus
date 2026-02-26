#include "FomodPlusScanner.h"

#include "FomodDbEntry.h"
#include "archiveparser.h"

#include <QDialog>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QVBoxLayout>
#include <archive.h>

#include <QLabel>
#include <QMovie>
#include <iostream>

#include "stringutil.h"

using ScanCallbackFn = std::function<bool(IModInterface*, ScanResult result)>;

bool FomodPlusScanner::init(IOrganizer* organizer)
{
    mOrganizer = organizer;

    mDialog = new QDialog();
    mDialog->setWindowTitle(tr("FOMOD Scanner"));

    const auto layout = new QVBoxLayout(mDialog);

    const QString description
        = tr("Greetings, traveler.\n"
             "This tool will scan your load order for mods installed via FOMOD.\n"
             "It will also fix up any erroneous FOMOD flags from previous versions of FOMOD Plus :) \n\n"
             "Safe travels, and may your load order be free of conflicts.");

    const auto descriptionLabel = new QLabel(description, mDialog);
    descriptionLabel->setWordWrap(true); // Enable word wrap for large text
    descriptionLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    descriptionLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop); // Align text to the top-left corner

    const auto gifLabel = new QLabel(mDialog);
    const auto movie    = new QMovie(":/fomod/wizard");
    gifLabel->setMovie(movie);
    movie->start();

    mProgressBar = new QProgressBar(mDialog);
    mProgressBar->setRange(0, mOrganizer->modList()->allMods().size());
    mProgressBar->setVisible(false);

    const auto scanButton   = new QPushButton(tr("Scan"), mDialog);
    const auto cancelButton = new QPushButton(tr("Cancel"), mDialog);

    layout->addWidget(descriptionLabel, 1);
    layout->addWidget(gifLabel, 1);
    layout->addWidget(mProgressBar, 1);
    layout->addWidget(scanButton, 1);
    layout->addWidget(cancelButton, 1);

    connect(cancelButton, &QPushButton::clicked, mDialog, &QDialog::reject);
    connect(scanButton, &QPushButton::clicked, this, &FomodPlusScanner::onScanClicked);
    connect(mDialog, &QDialog::finished, this, &FomodPlusScanner::cleanup);

    mDialog->setLayout(layout);
    mDialog->setMinimumSize(400, 300);
    mDialog->adjustSize();
    descriptionLabel->adjustSize();
    return true;
}

void FomodPlusScanner::onScanClicked() const
{
    mProgressBar->setVisible(true);
    const int added = scanLoadOrder(setFomodInfoForMod);
    mDialog->accept();
    QMessageBox::information(mDialog, tr("Scan Complete"),
        tr("The load order scan is complete. Updated filter info for ") + QString::number(added) + tr(" mods."));
    mOrganizer->refresh();
}

void FomodPlusScanner::cleanup() const
{
    mProgressBar->reset();
    mProgressBar->setVisible(false);
}

void FomodPlusScanner::display() const { mDialog->exec(); }

int FomodPlusScanner::scanLoadOrder(const ScanCallbackFn& callback) const
{
    int progress = 0;
    int modified = 0;
    for (const auto& modName : mOrganizer->modList()->allMods()) {
        const auto mod = mOrganizer->modList()->getMod(modName);
        if (const ScanResult result = openInstallationArchive(mod); callback(mod, result)) {
            modified++;
        }
        mProgressBar->setValue(++progress);
    }
    return modified;
}

ScanResult FomodPlusScanner::openInstallationArchive(const IModInterface* mod) const
{
    const auto downloadsDir         = mOrganizer->downloadsPath();
    const auto installationFilePath = mod->installationFile();
    return ArchiveParser::scanForFomodFiles(downloadsDir, installationFilePath, mod->name());
}

bool FomodPlusScanner::setFomodInfoForMod(IModInterface* mod, const ScanResult result)
{
    const auto pluginName = "FOMOD Plus";
    const auto setting    = mod->pluginSetting(pluginName, "fomod", 0);
    if (setting == 0 && ScanResult::HAS_FOMOD == result) {
        return mod->setPluginSetting(pluginName, "fomod", "{}");
    }
    if (setting != 0 && ScanResult::NO_FOMOD == result) {
        // Only clear if the existing setting is a bare flag ("{}") set by a previous scan.
        // Never clear rich JSON data written by the installer — those are user choices.
        if (setting.toString() == "{}") {
            return mod->setPluginSetting(pluginName, "fomod", 0);
        }
    }
    return false;
}

bool FomodPlusScanner::removeFomodInfoFromMod(IModInterface* mod, ScanResult)
{
    const auto pluginName = QString::fromStdString("FOMOD Plus");
    return mod->setPluginSetting(pluginName, "fomod", 0);
}
