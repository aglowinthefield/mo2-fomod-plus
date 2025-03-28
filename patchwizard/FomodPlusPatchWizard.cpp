#include "FomodPlusPatchWizard.h"

#include <QDialog>

bool FomodPlusPatchWizard::init(IOrganizer* organizer)
{
    mOrganizer = organizer;
    mDialog = new QDialog();
    mDialog->setWindowTitle(tr("Patch Wizard"));

    // Set up UI

    return true;
}

void FomodPlusPatchWizard::display() const
{
    mDialog->exec();
}