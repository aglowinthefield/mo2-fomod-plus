#include "FomodPlusScanner.h"

#include "FomodArchiveReader.h"

#include <QDialog>
#include <QPushButton>
#include <QProgressBar>
#include <QVBoxLayout>
#include <QMessageBox>
#include <archive.h>

#include <QLabel>
#include <QMovie>
#include <iostream>

#include "stringutil.h"

#include <QSettings>

using ScanCallbackFn = std::function<bool(IModInterface*, ArchiveScanResult result)>;

bool FomodPlusScanner::init(IOrganizer* organizer)
{
    mOrganizer = organizer;
    mReader    = std::make_unique<FomodArchiveReader>(organizer);

    mDialog = new QDialog();
    mDialog->setWindowTitle(tr("FOMOD Scanner"));

    const auto layout = new QVBoxLayout(mDialog);

    const QString description = tr("Greetings, traveler.\n"
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

    const auto scanButton     = new QPushButton(tr("Scan"), mDialog);
    const auto cancelButton   = new QPushButton(tr("Cancel"), mDialog);
    const auto deepScanButton = new QPushButton(tr("Deep Scan"), mDialog);

    layout->addWidget(descriptionLabel, 1);
    layout->addWidget(gifLabel, 1);
    layout->addWidget(mProgressBar, 1);
    layout->addWidget(scanButton, 1);
    layout->addWidget(cancelButton, 1);
    layout->addWidget(deepScanButton, 1);

    connect(cancelButton, &QPushButton::clicked, mDialog, &QDialog::reject);
    connect(scanButton, &QPushButton::clicked, this, &FomodPlusScanner::onScanClicked);
    connect(deepScanButton, &QPushButton::clicked, this, &FomodPlusScanner::onDeepScanClicked);
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

void FomodPlusScanner::onDeepScanClicked() const
{
    mProgressBar->setVisible(true);
    const int added = scanLoadOrder([&](IModInterface* mod, ArchiveScanResult result) {
        return populateDbForMod(mod, result);
    });
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

void FomodPlusScanner::display() const
{
    mDialog->exec();
}

int FomodPlusScanner::scanLoadOrder(const ScanCallbackFn& callback) const
{
    int progress = 0;
    int modified = 0;
    for (const auto& modName : mOrganizer->modList()->allMods()) {
        const auto mod    = mOrganizer->modList()->getMod(modName);
        const auto result = mReader->scanInstallationArchive(mod);
        if (callback(mod, result)) {
            modified++;
        }
        mProgressBar->setValue(++progress);
    }
    return modified;
}


/*
 * Somewhere we need to make and persist a map that looks like this:
 * {
 *      FOMODModName: [
 *          { "modName": "ModName", masters: ["Master1.esp", "Master2.esl"] }
 *      ]
 * }
 */
void collectMasters()
{
    const auto cwd = QDir::currentPath();
    QSettings settings(cwd + "/fomod-plus-settings.ini", QSettings::IniFormat);
}

bool FomodPlusScanner::populateDbForMod(IModInterface* mod, ArchiveScanResult result) const
{
    if (result != HAS_FOMOD) {
        return false;
    }
    const auto results = mReader->getModsWithMasters(mod);
    return true;
}

bool FomodPlusScanner::setFomodInfoForMod(IModInterface* mod, ArchiveScanResult result)
{
    const auto pluginName = "FOMOD Plus";
    const auto setting    = mod->pluginSetting(pluginName, "fomod", 0);
    if (setting == 0 && HAS_FOMOD == result) {
        return mod->setPluginSetting(pluginName, "fomod", "{}");
    }
    if (setting != 0 && NO_FOMOD == result) {
        return mod->setPluginSetting(pluginName, "fomod", 0);
    }
    return false;
}

bool FomodPlusScanner::removeFomodInfoFromMod(IModInterface* mod, ArchiveScanResult)
{
    const auto pluginName = QString::fromStdString("FOMOD Plus"); // TODO: can this be read from shared constants?
    // const auto settings = mod->clearPluginSettings(pluginName);
    // TODO Determine why this above doesnt work
    return mod->setPluginSetting(pluginName, "fomod", 0);
}