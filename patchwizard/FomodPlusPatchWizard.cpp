#include "FomodPlusPatchWizard.h"

#include "ModDetailsWidget.h"
#include "ModListItemModel.h"
#include "ifiletree.h"

#include <QHeaderView>
#include <QDialog>
#include <QLabel>
#include <QSplitter>
#include <iplugingame.h>
#include <qboxlayout.h>
#include <qtreeview.h>

const auto PLUGIN_EXTENSIONS = QStringList{ "esp", "esm", "esl" };

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
    return { 1, 0, 0, MOBase::VersionInfo::RELEASE_FINAL };
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

    connect(tree->selectionModel(), &QItemSelectionModel::selectionChanged, this,
        &FomodPlusPatchWizard::onSelectionChanged);
}

QTreeView* FomodPlusPatchWizard::createTreeView(QWidget* parent) const
{
    const auto tree  = new QTreeView(parent);
    const auto model = createModel();
    tree->setModel(model);
    tree->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    tree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    tree->header()->setStretchLastSection(false);
    return tree;
}

ModListItemModel* FomodPlusPatchWizard::createModel() const
{
    QList<std::shared_ptr<ModListItem> > modelItems;

    for (const auto mods = mOrganizer->modList()->allModsByProfilePriority(); const auto& thisMod : mods) {
        if (const auto mod = createModListItemForMod(thisMod); mod && !mod->pluginNames.empty()) {
            modelItems.append(mod);
        }
    }

    return new ModListItemModel(modelItems);
}

void FomodPlusPatchWizard::onSelectionChanged(const QItemSelection& selected, const QItemSelection& /*deselected*/) const
{
    if (selected.indexes().isEmpty()) {
        return;
    }

    const auto index   = selected.indexes().first();

    if (const auto modItem = static_cast<ModListItem*>(index.internalPointer())) {
        const auto tabView = mDialog->findChild<QTabWidget*>();
        // Update the QTabWidget with data from the selected modItem
        // For example, clear existing tabs and add new ones with relevant data
        tabView->clear();

        // Just list everything in its PluginToMentionsMap
        tabView->addTab(new ModDetailsWidget(modItem), tr("All Mentions"));
    }
}

std::vector<QString> getPluginNamesForMod(const MOBase::IModInterface* mod)
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

std::shared_ptr<ModListItem> FomodPlusPatchWizard::createModListItemForMod(const QString& mod) const
{
    const auto foundMod = mOrganizer->modList()->getMod(mod);
    if (!foundMod) {
        return nullptr;
    }
    if (foundMod->isOverwrite() || foundMod->isSeparator() || foundMod->isBackup()) {
        return nullptr;
    }
    if (mOrganizer->modList()->state(mod) & MOBase::IModList::STATE_ESSENTIAL) {
        logMessage(DEBUG, "Mod is essential, skipping: " + mod.toStdString());
        return nullptr;
    }

    const auto pluginNames = getPluginNamesForMod(foundMod);
    const auto map         = populatePatches(pluginNames);
    return std::make_shared<ModListItem>(foundMod, pluginNames, map);
}

std::vector<FomodNotes> FomodPlusPatchWizard::getAllModsWithFomodNotes() const
{
    const auto pluginName      = QString::fromStdString("FOMOD Plus");
    const auto& allModsQString = mOrganizer->modList()->allMods();
    std::vector allMods(allModsQString.begin(), allModsQString.end());
    std::vector<QString> fomodMods;

    // find all mods with FOMOD data in the LO
    auto hasFomod = [pluginName, this](const QString& modName) {
        const auto mod = mOrganizer->modList()->getMod(modName);
        return mod->pluginSetting(pluginName, "fomod", 0) != 0;
    };

    std::ranges::copy_if(allMods, std::back_inserter(fomodMods), hasFomod);

    std::vector<FomodNotes> fomodModsWithNotes;
    std::vector<std::optional<FomodNotes> > tempResults(fomodMods.size());

    std::ranges::transform(fomodMods, tempResults.begin(),
        [this, pluginName](const QString& modName) -> std::optional<FomodNotes> {
            const auto mod = mOrganizer->modList()->getMod(modName);
            if (const auto notes = mod->pluginSetting(pluginName, "installationNotes", "0"); notes != "0") {
                return parseFomodNotes(modName, notes.toString());
            }
            return std::nullopt;
        });

    for (const auto& notes : tempResults) {
        if (notes.has_value()) {
            fomodModsWithNotes.push_back(*notes);
        }
    }

    return fomodModsWithNotes;
}

PluginToMentionsMap FomodPlusPatchWizard::populatePatches(const std::vector<QString>& pluginNames) const
{
    PluginToMentionsMap map;

    const std::vector<FomodNotes> fomodMods = getAllModsWithFomodNotes();

    for (const auto& pluginName : pluginNames) {
        for (auto [modName, hasPatchFor, installedPatchFor, notInstalledPatchFor] : fomodMods) {
            bool mentions = false;
            if (std::ranges::find(hasPatchFor, pluginName) != hasPatchFor.end()) {
                mentions = true;
            }
            if (std::ranges::find(installedPatchFor, pluginName) != installedPatchFor.end()) {
                mentions = true;
            }
            if (std::ranges::find(notInstalledPatchFor, pluginName) != notInstalledPatchFor.end()) {
                mentions = true;
            }
            if (mentions) {
                if (const auto modPtr = mOrganizer->modList()->getMod(modName)) {
                    map[pluginName].push_back(modPtr);
                }
            }
        }
    }
    return map;
}

FomodNotes FomodPlusPatchWizard::parseFomodNotes(const QString& modName, const QString& notes)
{
    FomodNotes fomodNotes;
    fomodNotes.modName = modName;

    for (QStringList lines = notes.split('\n', Qt::SkipEmptyParts); const QString& line : lines) {
        if (line.startsWith("hasPatchFor:")) {
            fomodNotes.hasPatchFor.append(line.mid(QString("hasPatchFor:").length()).trimmed());
        } else if (line.startsWith("installedPatchFor:")) {
            fomodNotes.installedPatchFor.append(line.mid(QString("installedPatchFor:").length()).trimmed());
        } else if (line.startsWith("notInstalledPatchFor:")) {
            fomodNotes.notInstalledPatchFor.append(line.mid(QString("notInstalledPatchFor:").length()).trimmed());
        }
    }

    return fomodNotes;
}