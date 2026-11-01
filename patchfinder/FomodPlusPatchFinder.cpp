#include "FomodPlusPatchFinder.h"

#include <ifiletree.h>
#include <imodinterface.h>
#include <imoinfo.h>
#include <ipluginlist.h>
#include <stdexcept>

#include <QApplication>
#include <QDialog>
#include <QDir>
#include <QFile>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QProgressDialog>
#include <QPushButton>
#include <QScrollArea>
#include <QTreeWidget>
#include <QVBoxLayout>

using namespace MOBase;
using namespace std;

#pragma region Initialization

bool FomodPlusPatchFinder::init(IOrganizer* organizer)
{
  mOrganizer = organizer;
  mDialog    = new QDialog();
  mDialog->setWindowTitle(tr("Patch Finder"));
  mDialog->setMinimumSize(400, 200);
  log.setLogFilePath(QDir::currentPath().toStdString() + "/logs/fomodplus-patchfinder.log");

  PatchFinderInputs inputs;
  inputs.captureInstalled = [this] {
    PatchFinderInputs::InstalledState installed;
    auto* mods    = mOrganizer->modList();
    auto* plugins = mOrganizer->pluginList();
    if (!mods || !plugins)
      throw std::runtime_error(tr("The MO2 mod or plugin list is unavailable.").toStdString());
    for (const auto& modName : mods->allMods()) {
      const auto* mod = mods->getMod(modName);
      if (!mod)
        throw std::runtime_error(tr("Could not read mod: %1").arg(modName).toStdString());
      const auto tree = mod->fileTree();
      if (!tree)
        throw std::runtime_error(tr("Could not read files for mod: %1").arg(modName).toStdString());
      for (const auto& file : *tree) {
        if (!file)
          throw std::runtime_error(tr("Could not read files for mod: %1").arg(modName).toStdString());
        if (file->isFile() && isPluginFile(file->name()))
          installed.plugins.insert(file->name().toStdString());
      }
    }
    installed.resolve = [this, plugins](const std::string& fileName) -> std::string {
      if (!isPluginFile(fileName))
        return mOrganizer->resolvePath(QString::fromStdString(fileName)).isEmpty() ? "Missing" : "Active";
      const auto state = plugins->state(QString::fromStdString(fileName));
      if (state & IPluginList::STATE_ACTIVE)
        return "Active";
      if (state & IPluginList::STATE_INACTIVE)
        return "Inactive";
      return "Missing";
    };
    return installed;
  };
  inputs.reinstall = [this](int modId, const std::string& displayName) { return reinstall(modId, displayName); };
  inputs.rescan    = [this](FomodDB& workspace) { return rescan(workspace); };
  mPatchFinder     = std::make_unique<PatchFinder>(mOrganizer->basePath().toStdString(), std::move(inputs));
  mOrganizer->onUserInterfaceInitialized([this](QMainWindow*) {
    const auto result = mPatchFinder->apply({ PatchActionKind::Refresh, {} });
    if (!result.error.empty())
      logMessage(INFO, result.error);
  });
  return true;
}

#pragma endregion

#pragma region Navigation/UI
void FomodPlusPatchFinder::display() const
{
  const auto result = mPatchFinder->apply({ PatchActionKind::Refresh, {} });
  reportAction(result);
  if (!result.accepted)
    return;
  rebuild();
  if (!mDialog->isVisible())
    mDialog->exec();
}

void FomodPlusPatchFinder::reportAction(const PatchActionResult& result) const
{
  if (!result.error.empty() || !result.accepted) {
    QMessageBox::warning(mDialog, tr("Patch Finder"),
        result.error.empty() ? tr("This action is no longer available. Please retry.")
                             : QString::fromStdString(result.error));
  }
}

void FomodPlusPatchFinder::applyAction(PatchAction action) const
{
  const auto result = mPatchFinder->apply(action);
  reportAction(result);
  if (result.accepted)
    rebuild();
}

void FomodPlusPatchFinder::rebuild() const
{
  // Clear any existing layout
  if (mDialog->layout() != nullptr) {
    const auto clearLayout = [](auto&& self, QLayout* layout) -> void {
      while (auto* item = layout->takeAt(0)) {
        if (item->layout())
          self(self, item->layout());
        delete item->widget();
        delete item;
      }
    };
    clearLayout(clearLayout, mDialog->layout());
    delete mDialog->layout();
  }

  const auto view = mPatchFinder->view();
  if (view.trackedMods > 0 || view.freshness != PatchFreshness::Fresh)
    setupPatchList();
  else
    setupEmptyState();
  addFreshnessBanner(view);
}

void FomodPlusPatchFinder::addFreshnessBanner(const PatchView& view) const
{
  if (view.freshness == PatchFreshness::Fresh)
    return;
  auto* banner  = new QWidget(mDialog);
  auto* layout  = new QHBoxLayout(banner);
  auto* message = new QLabel((view.freshness == PatchFreshness::Stale
                                     ? tr("Suggestions are stale. Refresh failed; showing the last successful results.")
                                     : tr("Suggestions are unavailable. Refresh failed."))
          + "\n" + QString::fromStdString(view.diagnostic),
      banner);
  message->setWordWrap(true);
  layout->addWidget(message, 1);
  auto* retry = new QPushButton(tr("Retry"), banner);
  retry->setEnabled(view.canRefresh);
  connect(retry, &QPushButton::clicked, mDialog, [this] { applyAction({ PatchActionKind::Refresh, {} }); });
  layout->addWidget(retry);
  static_cast<QVBoxLayout*>(mDialog->layout())->insertWidget(0, banner);
}
// ── Empty State ─────────────────────────────────────────────────────────────

void FomodPlusPatchFinder::setupEmptyState() const
{
  auto* mainLayout = new QVBoxLayout(mDialog);
  mainLayout->setAlignment(Qt::AlignCenter);

  auto* contentWidget = new QWidget(mDialog);
  auto* contentLayout = new QHBoxLayout(contentWidget);
  contentLayout->setAlignment(Qt::AlignCenter);
  contentLayout->setSpacing(16);

  auto* imageLabel = new QLabel(contentWidget);
  imageLabel->setPixmap(QPixmap(":/fomod/infoscroll").scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));

  auto* textLabel
      = new QLabel(tr("Nothing of interest yet. The finder learns more as you\ninstall FOMODs, so check back later!"),
          contentWidget);
  textLabel->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);

  contentLayout->addWidget(imageLabel);
  contentLayout->addWidget(textLabel);

  mainLayout->addWidget(contentWidget);

  auto* rescanButton = new QPushButton(tr("Rescan Load Order"), mDialog);
  rescanButton->setEnabled(mPatchFinder->view().canRescan);
  connect(rescanButton, &QPushButton::clicked, const_cast<FomodPlusPatchFinder*>(this),
      &FomodPlusPatchFinder::onRescanClicked);
  mainLayout->addWidget(rescanButton, 0, Qt::AlignCenter);
}

// ── Main Patch List View ────────────────────────────────────────────────────

void FomodPlusPatchFinder::setupPatchList() const
{
  mDialog->setMinimumSize(800, 600);

  auto* mainLayout = new QVBoxLayout(mDialog);

  // Top bar with search and rescan button
  auto* topBar = new QHBoxLayout();

  auto* searchBox = new QLineEdit(mDialog);
  searchBox->setPlaceholderText(tr("Search patches..."));
  searchBox->setClearButtonEnabled(true);
  topBar->addWidget(searchBox, 1);

  auto* rescanButton = new QPushButton(tr("Rescan"), mDialog);
  rescanButton->setEnabled(mPatchFinder->view().canRescan);
  connect(rescanButton, &QPushButton::clicked, const_cast<FomodPlusPatchFinder*>(this),
      &FomodPlusPatchFinder::onRescanClicked);
  topBar->addWidget(rescanButton);

  mainLayout->addLayout(topBar);

  // Scroll area for the suggested patches list
  auto* scrollArea = new QScrollArea(mDialog);
  scrollArea->setWidgetResizable(true);
  scrollArea->setFrameShape(QFrame::NoFrame);

  auto* scrollContent = new QWidget();
  scrollContent->setObjectName("suggestedContent");
  auto* scrollLayout = new QVBoxLayout(scrollContent);
  scrollLayout->setAlignment(Qt::AlignTop);

  scrollArea->setWidget(scrollContent);
  mainLayout->addWidget(scrollArea, 1);

  // Populate the suggested list
  populateSuggested(scrollContent, {});

  // ── Browse All Section (collapsible) ────────────────────────────────────
  auto* browseGroup = new QGroupBox(tr("Browse all tracked options"), mDialog);
  browseGroup->setCheckable(true);
  browseGroup->setChecked(false);

  auto* browseLayout = new QVBoxLayout(browseGroup);

  auto* browseTree = new QTreeWidget(browseGroup);
  browseTree->setHeaderLabels({ tr("Name"), tr("Status"), tr("Plugin File") });
  browseTree->setAlternatingRowColors(true);
  browseTree->setRootIsDecorated(true);
  browseTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
  browseTree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
  browseTree->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
  browseTree->setVisible(false); // Hidden until expanded

  browseLayout->addWidget(browseTree);
  mainLayout->addWidget(browseGroup);

  // Reapply the current search whenever the hidden tree is expanded.
  connect(browseGroup, &QGroupBox::toggled, [browseTree, searchBox, this](bool checked) {
    browseTree->setVisible(checked);
    if (checked) {
      populateBrowseTree(browseTree, searchBox->text());
    }
  });

  // Connect search to rebuild both views
  connect(searchBox, &QLineEdit::textChanged, [scrollContent, browseTree, browseGroup, this](const QString& text) {
    populateSuggested(scrollContent, text);
    if (browseGroup->isChecked()) {
      populateBrowseTree(browseTree, text);
    }
  });

  // Stats label at the bottom
  const auto view  = mPatchFinder->view();
  auto* statsLabel = new QLabel(tr("%1 suggested patches across %2 mods")
                                    .arg(static_cast<qulonglong>(view.suggestedCount))
                                    .arg(static_cast<qulonglong>(view.suggestedModCount)),
      mDialog);
  statsLabel->setVisible(view.freshness != PatchFreshness::Unavailable);
  statsLabel->setAlignment(Qt::AlignRight);
  mainLayout->addWidget(statsLabel);
}

// ── Populate Suggested Patches ──────────────────────────────────────────────

void FomodPlusPatchFinder::populateSuggested(QWidget* container, const QString& filter) const
{
  // Clear existing content
  auto* layout = container->layout();
  if (layout != nullptr) {
    QLayoutItem* item;
    while ((item = layout->takeAt(0)) != nullptr) {
      delete item->widget();
      delete item;
    }
  }

  const auto view    = mPatchFinder->view({ true, filter.toStdString() });
  int totalSuggested = 0;
  for (const auto& mod : view.mods) {
    if (mod.options.empty())
      continue;
    totalSuggested += static_cast<int>(mod.options.size());

    // ── Mod Group Frame ─────────────────────────────────────────────
    auto* groupFrame = new QFrame(container);
    groupFrame->setFrameShape(QFrame::StyledPanel);
    groupFrame->setFrameShadow(QFrame::Raised);
    auto* groupLayout = new QVBoxLayout(groupFrame);
    groupLayout->setContentsMargins(8, 6, 8, 6);
    groupLayout->setSpacing(4);

    // Header row: mod name + reinstall button
    auto* headerRow    = new QHBoxLayout();
    auto* modNameLabel = new QLabel(QString::fromStdString(mod.displayName), groupFrame);
    auto font          = modNameLabel->font();
    font.setBold(true);
    modNameLabel->setFont(font);
    headerRow->addWidget(modNameLabel);

    headerRow->addStretch();

    auto* reinstallButton = new QPushButton(tr("Reinstall Mod"), groupFrame);
    reinstallButton->setFixedHeight(24);
    reinstallButton->setEnabled(view.canModify);
    const auto handle = mod.options.front().handle;
    connect(reinstallButton, &QPushButton::clicked, mDialog,
        [this, handle] { applyAction({ PatchActionKind::Reinstall, handle }); });
    headerRow->addWidget(reinstallButton);

    groupLayout->addLayout(headerRow);

    // Separator line under header
    auto* separator = new QFrame(groupFrame);
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Sunken);
    groupLayout->addWidget(separator);

    // ── Option Rows ─────────────────────────────────────────────────
    for (const auto& row : mod.options) {
      const auto* option = &row.option;
      auto* optionWidget = new QWidget(groupFrame);
      auto* optionLayout = new QVBoxLayout(optionWidget);
      optionLayout->setContentsMargins(16, 2, 4, 2);
      optionLayout->setSpacing(0);

      // Option name as primary label
      auto* nameLabel = new QLabel(QString::fromStdString(option->name), optionWidget);
      optionLayout->addWidget(nameLabel);

      // Plugin filename as subtitle
      if (!option->fileName.empty()) {
        auto* fileNameLabel = new QLabel(QString::fromStdString(option->fileName), optionWidget);
        auto fileFont       = fileNameLabel->font();
        fileFont.setPointSizeF(fileFont.pointSizeF() * 0.85);
        fileNameLabel->setFont(fileFont);
        fileNameLabel->setStyleSheet("color: gray;");
        optionLayout->addWidget(fileNameLabel);
      }

      // Masters in smaller, dimmer text
      if (!option->masters.empty()) {
        QString mastersStr;
        for (size_t i = 0; i < option->masters.size(); ++i) {
          if (i > 0)
            mastersStr += ", ";
          mastersStr += QString::fromStdString(option->masters[i]);
        }
        auto* mastersLabel = new QLabel(tr("Masters: %1").arg(mastersStr), optionWidget);
        auto mastersFont   = mastersLabel->font();
        mastersFont.setPointSizeF(mastersFont.pointSizeF() * 0.85);
        mastersLabel->setFont(mastersFont);
        mastersLabel->setStyleSheet("color: gray;");
        optionLayout->addWidget(mastersLabel);
      }

      // Enable right-click context menu for dismiss
      optionWidget->setContextMenuPolicy(Qt::CustomContextMenu);
      const auto optionHandle = row.handle;
      connect(optionWidget, &QWidget::customContextMenuRequested, mDialog,
          [this, optionHandle, optionWidget, canModify = view.canModify](const QPoint& pos) {
            QMenu menu;
            auto* dismissAction = menu.addAction(tr("Dismiss"));
            dismissAction->setEnabled(canModify);
            if (menu.exec(optionWidget->mapToGlobal(pos)) == dismissAction)
              applyAction({ PatchActionKind::Dismiss, optionHandle });
          });
      groupLayout->addWidget(optionWidget);
    }

    layout->addWidget(groupFrame);
  }

  // Empty state when nothing is suggested
  if (totalSuggested == 0 && view.freshness == PatchFreshness::Fresh) {
    auto* emptyLabel = new QLabel(filter.isEmpty()
            ? tr("No suggested patches found. All your patches are installed,\nor run Rescan to check for new ones.")
            : tr("No patches match your search."),
        container);
    emptyLabel->setAlignment(Qt::AlignCenter);
    emptyLabel->setStyleSheet("color: gray; padding: 40px;");
    layout->addWidget(emptyLabel);
  }
}

// ── Browse All Tree ─────────────────────────────────────────────────────────

void FomodPlusPatchFinder::populateBrowseTree(QTreeWidget* tree, const QString& filter) const
{
  tree->clear();

  const auto view = mPatchFinder->view({ false, filter.toStdString() });
  for (const auto& mod : view.mods) {
    // Mod-level item
    auto* modItem = new QTreeWidgetItem(tree);
    modItem->setText(0, QString::fromStdString(mod.displayName));
    modItem->setExpanded(!filter.isEmpty());

    // Option items (flat — 2 levels total)
    for (const auto& row : mod.options) {
      const auto* option = &row.option;
      auto* optionItem   = new QTreeWidgetItem(modItem);
      optionItem->setText(0, QString::fromStdString(option->name));

      // Status column
      QString status;
      const bool suggested = row.suggested;
      if (suggested) {
        status = tr("Suggested");
        optionItem->setForeground(1, QBrush(Qt::darkCyan));
      } else {
        switch (option->selectionState) {
        case SelectionState::Selected:
          status = tr("Selected");
          optionItem->setForeground(1, QBrush(Qt::darkGreen));
          break;
        case SelectionState::Deselected:
          status = tr("Deselected");
          optionItem->setForeground(1, QBrush(Qt::darkRed));
          break;
        case SelectionState::Available:
          status = tr("Available");
          optionItem->setForeground(1, QBrush(Qt::darkGray));
          break;
        default:
          status = tr("Unknown");
          break;
        }
      }
      optionItem->setText(1, status);

      // Plugin file column
      if (!option->fileName.empty()) {
        optionItem->setText(2, QString::fromStdString(option->fileName));
      }

      // Tooltip with step/group and masters
      QStringList tooltipParts;
      if (!option->step.empty() || !option->group.empty()) {
        tooltipParts << QString::fromStdString(option->step + " > " + option->group);
      }
      if (!option->masters.empty()) {
        QString mastersStr = tr("Masters: ");
        for (size_t i = 0; i < option->masters.size(); ++i) {
          if (i > 0)
            mastersStr += ", ";
          mastersStr += QString::fromStdString(option->masters[i]);
        }
        tooltipParts << mastersStr;
      }
      if (!tooltipParts.isEmpty()) {
        const auto tooltip = tooltipParts.join("\n");
        optionItem->setToolTip(0, tooltip);
        optionItem->setToolTip(2, tooltip);
      }
    }
  }
}

// ── Reinstall Action ────────────────────────────────────────────────────────

bool FomodPlusPatchFinder::reinstall(int modId, const std::string& displayName)
{
  logMessage(DEBUG, "Reinstall requested for: " + displayName);

  if (!mOrganizer->modList())
    throw std::runtime_error(tr("The MO2 mod or plugin list is unavailable.").toStdString());

  // Look up the mod by Nexus mod ID first, fall back to display name
  MOBase::IModInterface* targetMod = nullptr;
  const int targetModId            = modId;

  for (const auto& modName : mOrganizer->modList()->allMods()) {
    auto* mod = mOrganizer->modList()->getMod(modName);
    if (mod == nullptr)
      continue;

    if (targetModId > 0 && mod->nexusId() == targetModId) {
      targetMod = mod;
      break;
    }
  }

  // Fall back to display name matching
  if (targetMod == nullptr) {
    const auto targetName = QString::fromStdString(displayName);
    for (const auto& modName : mOrganizer->modList()->allMods()) {
      auto* mod = mOrganizer->modList()->getMod(modName);
      if (mod != nullptr && mod->name() == targetName) {
        targetMod = mod;
        break;
      }
    }
  }

  if (targetMod == nullptr) {
    QMessageBox::warning(mDialog, tr("Mod Not Found"),
        tr("Could not find \"%1\" in your mod list.").arg(QString::fromStdString(displayName)));
    return false;
  }

  // Get the archive path
  const auto installationFile = targetMod->installationFile();
  if (installationFile.isEmpty()) {
    QMessageBox::warning(mDialog, tr("Archive Not Found"),
        tr("No archive file is associated with \"%1\".\nThe download may have been deleted.")
            .arg(QString::fromStdString(displayName)));
    return false;
  }

  // Resolve relative paths against downloads directory
  const auto archivePath
      = QDir(installationFile).isAbsolute() ? installationFile : mOrganizer->downloadsPath() + "/" + installationFile;

  if (!QFile::exists(archivePath)) {
    QMessageBox::warning(mDialog, tr("Archive Not Found"),
        tr("Archive file not found:\n%1\n\nRe-download from Nexus to reinstall.").arg(archivePath));
    return false;
  }

  logMessage(INFO, "Triggering reinstall from: " + archivePath.toStdString());

  // Close the Patch Finder dialog before triggering install
  // (the FOMOD installer will open its own dialog)
  mDialog->accept();

  // Trigger the installer — MO2 will route this through our FOMOD Plus installer
  return mOrganizer->installMod(archivePath) != nullptr;
}

#pragma endregion

#pragma region Rescan
// ── Rescan ──────────────────────────────────────────────────────────────────

void FomodPlusPatchFinder::onRescanClicked()
{
  if (!mPatchFinder || !mPatchFinder->view().canRescan)
    return;

  const auto confirmResult = QMessageBox::question(mDialog, tr("Rescan Load Order"),
      tr("Rescanning will populate as many existing choices and options as we can, "
         "but if some downloads are deleted it may be missing things! "
         "It may take a few minutes depending on the size of your load order and all that."),
      QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel);

  // Confirmation also pumps events; recheck before touching an active scan's state.
  if (confirmResult != QMessageBox::Ok || !mPatchFinder->view().canRescan) {
    return;
  }

  logMessage(DEBUG, "Rescan requested by user");

  mRescanResult.reset();
  const auto action = mPatchFinder->apply({ PatchActionKind::Rescan, {} });
  // Own the summary before showing dialogs that can dispatch another action.
  const auto rescanResult = std::move(mRescanResult);
  mRescanResult.reset();
  reportAction(action);
  if (!action.accepted)
    return;
  rebuild();
  if (action.completed && rescanResult) {
    const auto& result = *rescanResult;
    // Show result summary
    QString summary = tr("Rescan complete!\n\n"
                         "FOMODs scanned: %1\n"
                         "Skipped (no options): %2\n"
                         "Missing archives: %3\n"
                         "Parse errors: %4")
                          .arg(result.successfullyScanned)
                          .arg(result.skippedNoOptions)
                          .arg(result.missingArchives)
                          .arg(result.parseErrors);

    if (!result.failedMods.empty() && result.failedMods.size() <= 10) {
      summary += tr("\n\nIssues:");
      for (const auto& mod : result.failedMods) {
        summary += QString("\n- %1").arg(QString::fromStdString(mod));
      }
    } else if (result.failedMods.size() > 10) {
      summary += tr("\n\n%1 mods had issues (see log for details)").arg(result.failedMods.size());
      for (const auto& mod : result.failedMods) {
        logMessage(INFO, "Issue: " + mod);
      }
    }

    QMessageBox::information(mDialog, tr("Rescan Complete"), summary);

    logMessage(INFO,
        "Rescan complete: " + std::to_string(result.successfullyScanned) + " scanned, "
            + std::to_string(result.skippedNoOptions) + " skipped (no options), "
            + std::to_string(result.missingArchives) + " missing archives, " + std::to_string(result.parseErrors)
            + " parse errors");
  }
}

bool FomodPlusPatchFinder::rescan(FomodDB& workspace)
{
  if (!mOrganizer->modList())
    throw std::runtime_error(tr("The MO2 mod or plugin list is unavailable.").toStdString());

  // Exceptions from the progress callback unwind the scan without saving its private workspace.
  struct ScanCancelled { };
  QProgressDialog progress(tr("Scanning mods..."), tr("Cancel"), 0, 100, mDialog);
  progress.setWindowModality(Qt::WindowModal);
  progress.setMinimumDuration(0);
  progress.setAutoClose(false);
  progress.setAutoReset(false);
  progress.setValue(0);
  FomodRescan scanner(mOrganizer, &workspace);
  try {
    auto result = scanner.scanAllModsWithChoices(
        [&](int current, int total, const QString& modName) {
          progress.setValue(total > 0 ? current * 100 / total : 0);
          progress.setLabelText(tr("Scanning: %1 (%2/%3)").arg(modName).arg(current).arg(total));
          QApplication::processEvents();
          if (progress.wasCanceled())
            throw ScanCancelled {};
        },
        false);
    QApplication::processEvents();
    if (progress.wasCanceled())
      throw ScanCancelled {};
    progress.setValue(100);
    mRescanResult = std::move(result);
    return true;
  } catch (const ScanCancelled&) {
    QMessageBox::information(mDialog, tr("Rescan Cancelled"), tr("The rescan was cancelled. No changes were saved."));
    logMessage(INFO, "Rescan cancelled by user");
    return false;
  }
}
#pragma endregion
