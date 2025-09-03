#include "FomodPlusScanner.h"

#include <QDialog>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QVBoxLayout>
#include <archive/archive.h>

#include <QLabel>
#include <QMovie>
#include <iostream>

#include <fomod_plus_shared/stringutil.h>

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
        const auto mod          = mOrganizer->modList()->getMod(modName);
        const ScanResult result = openInstallationArchive(mod);
        if (callback(mod, result)) {
            modified++;
        }
        mProgressBar->setValue(++progress);
    }
    return modified;
}

bool hasFomodFiles(const std::vector<FileData*>& files)
{
    bool hasModuleXml = false;
    bool hasInfoXml   = false;

    for (const auto* file : files) {
        if (endsWithCaseInsensitive(file->getArchiveFilePath(), StringConstants::FomodFiles::W_MODULE_CONFIG.data())) {
            hasModuleXml = true;
        }
        if (endsWithCaseInsensitive(file->getArchiveFilePath(), StringConstants::FomodFiles::W_INFO_XML.data())) {
            hasInfoXml = true;
        }
    }
    return hasModuleXml && hasInfoXml;
}

std::ostream& operator<<(std::ostream& os, const Archive::Error& error)
{
    switch (error) {
    case Archive::Error::ERROR_NONE:
        os << "No error";
        break;
    case Archive::Error::ERROR_ARCHIVE_NOT_FOUND:
        os << "File not found";
        break;
    case Archive::Error::ERROR_FAILED_TO_OPEN_ARCHIVE:
        os << "Failed to open file";
        break;
    case Archive::Error::ERROR_INVALID_ARCHIVE_FORMAT:
        os << "Invalid archive format";
        break;
    default:
        os << "Unknown error??";
    }
    return os;
}

ScanResult FomodPlusScanner::openInstallationArchive(const IModInterface* mod) const
{
    const auto downloadsDir         = mOrganizer->downloadsPath();
    const auto installationFilePath = mod->installationFile();

    if (installationFilePath.isEmpty()) {
        return NO_ARCHIVE;
    }

    const auto qualifiedInstallerPath
        = QDir(installationFilePath).isAbsolute() ? installationFilePath : downloadsDir + "/" + installationFilePath;

    const auto archive = CreateArchive();

    if (!archive->isValid()) {
        std::cerr << "[" << mod->name().toStdString()
                  << "] Failed to load the archive module: " << archive->getLastError() << std::endl;
        return NO_ARCHIVE;
    }

    // Open the archive:
    if (!archive->open(qualifiedInstallerPath.toStdWString(), nullptr)) {
        std::cerr << "[" << mod->name().toStdString() << "] Failed to open the archive ["
                  << qualifiedInstallerPath.toStdString() << "]: " << archive->getLastError() << std::endl;
        return NO_ARCHIVE;
    }

    // Mark all files for extraction to their path in the archive:
    ScanResult result;
    if (hasFomodFiles(archive->getFileList())) {
        std::cout << "Found FOMOD files in " << qualifiedInstallerPath.toStdString() << std::endl;
        result = HAS_FOMOD;
    } else {
        result = NO_FOMOD;
    }

    // Explicitly close archive to free resources
    archive->close();
    return result;
}

bool FomodPlusScanner::setFomodInfoForMod(IModInterface* mod, ScanResult result)
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

bool FomodPlusScanner::removeFomodInfoFromMod(IModInterface* mod, ScanResult)
{
    const auto pluginName = QString::fromStdString("FOMOD Plus"); // TODO: can this be read from shared constants?
    // const auto settings = mod->clearPluginSettings(pluginName);
    // TODO Determine why this above doesnt work
    return mod->setPluginSetting(pluginName, "fomod", 0);
}
