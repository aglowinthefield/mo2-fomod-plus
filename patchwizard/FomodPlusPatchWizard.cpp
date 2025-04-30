#include "FomodPlusPatchWizard.h"

#include <QDialog>
#include "lib/PatchFinder.h"

bool FomodPlusPatchWizard::init(IOrganizer* organizer)
{
    mOrganizer = organizer;
    mDialog = new QDialog();
    mDialog->setWindowTitle(tr("Patch Wizard"));
    log.setLogFilePath(QDir::currentPath().toStdString() + "/logs/fomodplus-patchwizard.log");

    mOrganizer->onUserInterfaceInitialized([this](QMainWindow*){
        logMessage(DEBUG, "patches populated.");
        mPatchFinder = std::make_unique<PatchFinder>(mOrganizer);
        mPatchFinder->populateInstalledPlugins();
        const auto availablePatches = mPatchFinder->getAvailablePatchesForModList(); // TODO: JUST FOR TESTING
        std::cout << "Available Patches: " << availablePatches.size() << std::endl;
    });


    // Set up UI

    return true;
}

void FomodPlusPatchWizard::display() const
{
    mDialog->exec();
}