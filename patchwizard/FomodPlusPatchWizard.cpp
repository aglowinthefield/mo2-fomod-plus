#include "FomodPlusPatchWizard.h"

#include <QDialog>

bool FomodPlusPatchWizard::init(MOBase::IOrganizer* organizer)
{
    mOrganizer = organizer;
    return true;
}

QString FomodPlusPatchWizard::name() const
{
    return tr("Patch Wizard");
}

QString FomodPlusPatchWizard::author() const
{
    return "aglowinthefield";
}

QString FomodPlusPatchWizard::description() const
{
    return tr("Find patches you may want to install from FOMODs in your list");
}

MOBase::VersionInfo FomodPlusPatchWizard::version() const
{
    return MOBase::VersionInfo(1, 0, 0, MOBase::VersionInfo::RELEASE_FINAL);
}

QList<MOBase::PluginSetting> FomodPlusPatchWizard::settings() const
{
    return {};
}

QString FomodPlusPatchWizard::displayName() const
{
    return tr("Patch Wizard");
}

QString FomodPlusPatchWizard::tooltip() const
{
    return "";
}

QIcon FomodPlusPatchWizard::icon() const
{
    return QIcon(":/fomod/hat");
}

void FomodPlusPatchWizard::display() const
{
    if (mDialog) {
        mDialog->exec();
    }
}