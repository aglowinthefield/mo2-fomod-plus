#include "FomodPlusPatchWizard.h"

#include <QDialog>
#include "lib/PatchFinder.h"

bool FomodPlusPatchWizard::init(IOrganizer* organizer)
{
    mOrganizer = organizer;
    mDialog = new QDialog();
    mDialog->setWindowTitle(tr("Patch Wizard"));
    log.setLogFilePath(QDir::currentPath().toStdString() + "/logs/fomodplus-patchwizard.log");

    mPatchFinder = std::make_unique<PatchFinder>(mOrganizer);

    // Set up UI

    return true;
}

void FomodPlusPatchWizard::display() const
{
    mDialog->exec();
}