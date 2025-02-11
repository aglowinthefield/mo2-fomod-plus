#include "FomodPlusPatchWizard.h"

#include "ModListItemModel.h"
#include "ifiletree.h"

#include <QHeaderView>
#include <QDialog>
#include <QSplitter>
#include <qboxlayout.h>
#include <qtreeview.h>

const QStringList PLUGIN_EXTENSIONS = QStringList{ "esp", "esm", "esl" };

bool FomodPlusPatchWizard::init(MOBase::IOrganizer* organizer)
{
    mOrganizer = organizer;
    log.setLogFilePath(QDir::currentPath().toStdString() + "/logs/fp-patch-wizard.log");
    mOrganizer->onUserInterfaceInitialized([this](QMainWindow*) {
        createDialog();
    });
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

void FomodPlusPatchWizard::createDialog()
{
    mDialog = new QDialog();
    mDialog->setWindowTitle(tr("Patch Wizard"));

    const auto splitter = new QSplitter(mDialog);

    // left pane is a QTreeView of mods -> plugins
    const auto tree = createTreeView(mDialog);
    splitter->addWidget(tree);

    // right pane is a QTabView, one tab for now with 'mentions'
    const auto tabView = new QTabWidget(mDialog);
    splitter->addWidget(tabView);

    splitter->setSizes({ 1, 1 });
    splitter->setStretchFactor(0, 1); // left pane
    splitter->setStretchFactor(1, 1); // right pane

    const auto layout = new QVBoxLayout(mDialog);
    layout->addWidget(splitter);
    mDialog->setLayout(layout);

    mDialog->setMinimumSize(1000, 450);
    mDialog->adjustSize();
}

QTreeView* FomodPlusPatchWizard::createTreeView(QWidget* parent)
{
    const auto tree  = new QTreeView(parent);
    const auto model = createModel();
    tree->setModel(model);
    tree->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    tree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    return tree;
}

ModListItemModel* FomodPlusPatchWizard::createModel()
{
    QList<std::shared_ptr<ModListItem> > modelItems;
    const auto allMods = mOrganizer->modList()->allModsByProfilePriority();

    for (const auto& thisMod : allMods) {
        const auto mod = createModListItemForMod(thisMod);
        if (mod && mod->pluginNames.size() > 0) {
            modelItems.append(mod);
        }
    }

    return new ModListItemModel(modelItems);
}

std::vector<QString> getPluginNamesForMod(MOBase::IModInterface* mod)
{
    std::vector<QString> pluginNames;
    const auto tree = mod->fileTree();

    for (auto it = tree->begin(); it != tree->end(); ++it) {
        // this could be a directory too. ugh
        const auto entry     = *it;
        const auto entryName = entry->name().toLower();
        for (const auto& extension : PLUGIN_EXTENSIONS) {
            if (entryName.endsWith(extension.toLower())) {
                pluginNames.emplace_back(entryName);
            }
        }
    }
    return pluginNames;
}

std::shared_ptr<ModListItem> FomodPlusPatchWizard::createModListItemForMod(const QString& mod)
{
    const auto foundMod = mOrganizer->modList()->getMod(mod);
    if (!foundMod) {
        return nullptr;
    }
    if (foundMod->isOverwrite() || foundMod->isSeparator() || foundMod->isBackup()) {
        return nullptr;
    }

    const auto pluginNames = getPluginNamesForMod(foundMod);
    return std::make_shared<ModListItem>(foundMod, pluginNames);
}

PluginToMentionsMap FomodPlusPatchWizard::populatePatches(std::shared_ptr<ModListItem> item)
{
    PluginToMentionsMap map;

    for (const auto & pluginName : item->pluginNames) {

    }

    return map;
}