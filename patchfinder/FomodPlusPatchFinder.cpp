#include "FomodPlusPatchFinder.h"

#include <QApplication>
#include <QDialog>
#include <QDir>
#include <QFile>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QProgressDialog>
#include <QPushButton>
#include <QGroupBox>
#include <QHeaderView>
#include <QScrollArea>
#include <QTreeWidget>
#include <QVBoxLayout>

#include "lib/PatchFinder.h"
#include <FomodRescan.h>

// ── Init & Display ──────────────────────────────────────────────────────────

bool FomodPlusPatchFinder::init(IOrganizer* organizer)
{
    mOrganizer = organizer;
    mDialog    = new QDialog();
    mDialog->setWindowTitle(tr("Patch Finder"));
    mDialog->setMinimumSize(400, 200);
    log.setLogFilePath(QDir::currentPath().toStdString() + "/logs/fomodplus-patchfinder.log");

    mOrganizer->onUserInterfaceInitialized([this](QMainWindow*) {
        logMessage(DEBUG, "patches populated.");
        mPatchFinder = std::make_unique<PatchFinder>(mOrganizer);
        mPatchFinder->populateInstalledPlugins();
        mAvailablePatches = mPatchFinder->getAvailablePatchesForModList();
        loadDismissed();
        logMessage(DEBUG, "Available Patches: " + std::to_string(mAvailablePatches.size()));
    });

    return true;
}

void FomodPlusPatchFinder::display() const
{
    // Clear any existing layout
    if (mDialog->layout() != nullptr) {
        QLayoutItem* item;
        while ((item = mDialog->layout()->takeAt(0)) != nullptr) {
            delete item->widget();
            delete item;
        }
        delete mDialog->layout();
    }

    // Check if we have any fomod.db entries
    const bool hasEntries = !mPatchFinder->mFomodDb->getEntries().empty();

    if (hasEntries) {
        setupPatchList();
    } else {
        setupEmptyState();
    }

    mDialog->exec();
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

    // Toggle browse tree visibility and populate on first expand
    connect(browseGroup, &QGroupBox::toggled, [browseTree, this](bool checked) {
        browseTree->setVisible(checked);
        if (checked && browseTree->topLevelItemCount() == 0) {
            populateBrowseTree(browseTree, {});
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
    const auto& entries = mPatchFinder->mFomodDb->getEntries();
    int suggestedCount  = 0;
    int modCount        = 0;
    for (const auto& entry : entries) {
        bool modHasSuggested = false;
        for (const auto& option : entry->getOptions()) {
            const auto& dismissKey = option.fileName.empty() ? option.name : option.fileName;
            if (mPatchFinder->isSuggested(option) && !isDismissed(entry->getModId(), dismissKey)) {
                ++suggestedCount;
                modHasSuggested = true;
            }
        }
        if (modHasSuggested)
            ++modCount;
    }
    auto* statsLabel
        = new QLabel(tr("%1 suggested patches across %2 mods").arg(suggestedCount).arg(modCount), mDialog);
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

    const auto& entries       = mPatchFinder->mFomodDb->getEntries();
    const QString lowerFilter = filter.toLower();
    int totalSuggested        = 0;

    for (const auto& entry : entries) {
        // Collect suggested, non-dismissed options for this mod
        std::vector<const FomodOption*> suggestedOptions;

        for (const auto& option : entry->getOptions()) {
            if (!mPatchFinder->isSuggested(option))
                continue;
            const auto& optionDismissKey = option.fileName.empty() ? option.name : option.fileName;
            if (isDismissed(entry->getModId(), optionDismissKey))
                continue;

            // Apply search filter
            if (!filter.isEmpty()) {
                bool matches = QString::fromStdString(option.name).toLower().contains(lowerFilter)
                    || QString::fromStdString(option.fileName).toLower().contains(lowerFilter)
                    || QString::fromStdString(entry->getDisplayName()).toLower().contains(lowerFilter);

                if (!matches) {
                    for (const auto& master : option.masters) {
                        if (QString::fromStdString(master).toLower().contains(lowerFilter)) {
                            matches = true;
                            break;
                        }
                    }
                }
                if (!matches)
                    continue;
            }

            suggestedOptions.push_back(&option);
        }

        if (suggestedOptions.empty())
            continue;

        totalSuggested += static_cast<int>(suggestedOptions.size());

        // ── Mod Group Frame ─────────────────────────────────────────────
        auto* groupFrame = new QFrame(container);
        groupFrame->setFrameShape(QFrame::StyledPanel);
        groupFrame->setFrameShadow(QFrame::Raised);
        auto* groupLayout = new QVBoxLayout(groupFrame);
        groupLayout->setContentsMargins(8, 6, 8, 6);
        groupLayout->setSpacing(4);

        // Header row: mod name + reinstall button
        auto* headerRow    = new QHBoxLayout();
        auto* modNameLabel = new QLabel(QString::fromStdString(entry->getDisplayName()), groupFrame);
        auto font          = modNameLabel->font();
        font.setBold(true);
        modNameLabel->setFont(font);
        headerRow->addWidget(modNameLabel);

        headerRow->addStretch();

        auto* reinstallButton = new QPushButton(tr("Reinstall Mod"), groupFrame);
        reinstallButton->setFixedHeight(24);
        const auto* entryPtr = entry.get();
        connect(reinstallButton, &QPushButton::clicked, [this, entryPtr]() {
            const_cast<FomodPlusPatchFinder*>(this)->onReinstallClicked(entryPtr);
        });
        headerRow->addWidget(reinstallButton);

        groupLayout->addLayout(headerRow);

        // Separator line under header
        auto* separator = new QFrame(groupFrame);
        separator->setFrameShape(QFrame::HLine);
        separator->setFrameShadow(QFrame::Sunken);
        groupLayout->addWidget(separator);

        // ── Option Rows ─────────────────────────────────────────────────
        for (const auto* option : suggestedOptions) {
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
            const int modId            = entry->getModId();
            const std::string fileName = option->fileName.empty() ? option->name : option->fileName;
            connect(optionWidget, &QWidget::customContextMenuRequested,
                [this, modId, fileName, optionWidget](const QPoint& pos) {
                    QMenu menu;
                    auto* dismissAction = menu.addAction(tr("Dismiss"));
                    if (menu.exec(optionWidget->mapToGlobal(pos)) == dismissAction) {
                        const_cast<FomodPlusPatchFinder*>(this)->onDismissClicked(modId, fileName);
                    }
                });

            groupLayout->addWidget(optionWidget);
        }

        layout->addWidget(groupFrame);
    }

    // Empty state when nothing is suggested
    if (totalSuggested == 0) {
        auto* emptyLabel = new QLabel(
            filter.isEmpty()
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

    const auto& entries       = mPatchFinder->mFomodDb->getEntries();
    const QString lowerFilter = filter.toLower();

    for (const auto& entry : entries) {
        // Collect matching options (flat — no step/group nesting)
        std::vector<const FomodOption*> matchingOptions;
        for (const auto& option : entry->getOptions()) {
            if (!filter.isEmpty()) {
                bool matches = QString::fromStdString(option.name).toLower().contains(lowerFilter)
                    || QString::fromStdString(option.fileName).toLower().contains(lowerFilter)
                    || QString::fromStdString(entry->getDisplayName()).toLower().contains(lowerFilter);

                if (!matches) {
                    for (const auto& master : option.masters) {
                        if (QString::fromStdString(master).toLower().contains(lowerFilter)) {
                            matches = true;
                            break;
                        }
                    }
                }
                if (!matches)
                    continue;
            }
            matchingOptions.push_back(&option);
        }

        if (matchingOptions.empty())
            continue;

        // Mod-level item
        auto* modItem = new QTreeWidgetItem(tree);
        modItem->setText(0, QString::fromStdString(entry->getDisplayName()));
        modItem->setExpanded(!filter.isEmpty());

        // Option items (flat — 2 levels total)
        for (const auto* option : matchingOptions) {
            auto* optionItem = new QTreeWidgetItem(modItem);
            optionItem->setText(0, QString::fromStdString(option->name));

            // Status column
            QString status;
            const bool suggested = mPatchFinder->isSuggested(*option);
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

void FomodPlusPatchFinder::onReinstallClicked(const FomodDbEntry* entry)
{
    logMessage(DEBUG, "Reinstall requested for: " + entry->getDisplayName());

    // Look up the mod by Nexus mod ID first, fall back to display name
    MOBase::IModInterface* targetMod = nullptr;
    const int targetModId            = entry->getModId();

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
        const auto displayName = QString::fromStdString(entry->getDisplayName());
        for (const auto& modName : mOrganizer->modList()->allMods()) {
            auto* mod = mOrganizer->modList()->getMod(modName);
            if (mod != nullptr && mod->name() == displayName) {
                targetMod = mod;
                break;
            }
        }
    }

    if (targetMod == nullptr) {
        QMessageBox::warning(mDialog, tr("Mod Not Found"),
            tr("Could not find \"%1\" in your mod list.").arg(QString::fromStdString(entry->getDisplayName())));
        return;
    }

    // Get the archive path
    const auto installationFile = targetMod->installationFile();
    if (installationFile.isEmpty()) {
        QMessageBox::warning(mDialog, tr("Archive Not Found"),
            tr("No archive file is associated with \"%1\".\nThe download may have been deleted.")
                .arg(QString::fromStdString(entry->getDisplayName())));
        return;
    }

    // Resolve relative paths against downloads directory
    const auto archivePath = QDir(installationFile).isAbsolute()
        ? installationFile
        : mOrganizer->downloadsPath() + "/" + installationFile;

    if (!QFile::exists(archivePath)) {
        QMessageBox::warning(mDialog, tr("Archive Not Found"),
            tr("Archive file not found:\n%1\n\nRe-download from Nexus to reinstall.").arg(archivePath));
        return;
    }

    logMessage(INFO, "Triggering reinstall from: " + archivePath.toStdString());

    // Close the Patch Finder dialog before triggering install
    // (the FOMOD installer will open its own dialog)
    mDialog->accept();

    // Trigger the installer — MO2 will route this through our FOMOD Plus installer
    mOrganizer->installMod(archivePath);

    // Refresh data after reinstall — reload the DB from disk since the
    // installer wrote updated selection states to a separate FomodDB instance
    mPatchFinder->mFomodDb->reload();
    mPatchFinder->populateInstalledPlugins();
    mAvailablePatches = mPatchFinder->getAvailablePatchesForModList();
}

// ── Dismiss ─────────────────────────────────────────────────────────────────

void FomodPlusPatchFinder::onDismissClicked(const int modId, const std::string& fileName)
{
    const auto key = makeDismissKey(modId, fileName);
    mDismissedPatches.insert(key);
    saveDismissed();
    logMessage(DEBUG, "Dismissed patch: " + key);

    // Refresh the UI in place by re-calling display()
    display();
}

std::string FomodPlusPatchFinder::getDismissedFilePath() const
{
    return (std::filesystem::path(mOrganizer->basePath().toStdString()) / "fomod-dismissed.json").string();
}

void FomodPlusPatchFinder::loadDismissed()
{
    mDismissedPatches.clear();
    const auto path = getDismissedFilePath();

    if (!std::filesystem::exists(path))
        return;

    try {
        std::ifstream file(path);
        if (!file.is_open())
            return;

        auto json = nlohmann::json::parse(file);
        if (!json.is_array())
            return;

        for (const auto& item : json) {
            if (item.is_string()) {
                mDismissedPatches.insert(item.get<std::string>());
            }
        }
        logMessage(DEBUG, "Loaded " + std::to_string(mDismissedPatches.size()) + " dismissed patches");
    } catch ([[maybe_unused]] const std::exception& e) {
        logMessage(INFO, "Failed to load dismissed patches file");
    }
}

void FomodPlusPatchFinder::saveDismissed() const
{
    try {
        nlohmann::json json = nlohmann::json::array();
        for (const auto& key : mDismissedPatches) {
            json.push_back(key);
        }

        std::ofstream file(getDismissedFilePath());
        if (file.is_open()) {
            file << json.dump(2);
        }
    } catch ([[maybe_unused]] const std::exception& e) {
        logMessage(INFO, "Failed to save dismissed patches file");
    }
}

bool FomodPlusPatchFinder::isDismissed(const int modId, const std::string& fileName) const
{
    return mDismissedPatches.contains(makeDismissKey(modId, fileName));
}

std::string FomodPlusPatchFinder::makeDismissKey(const int modId, const std::string& fileName)
{
    return std::to_string(modId) + ":" + fileName;
}

// ── Rescan ──────────────────────────────────────────────────────────────────

void FomodPlusPatchFinder::onRescanClicked()
{
    const auto confirmResult = QMessageBox::question(mDialog, tr("Rescan Load Order"),
        tr("Rescanning will populate as many existing choices and options as we can, "
           "but if some downloads are deleted it may be missing things! "
           "It may take a few minutes depending on the size of your load order and all that."),
        QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel);

    if (confirmResult != QMessageBox::Ok) {
        return;
    }

    logMessage(DEBUG, "Rescan requested by user");

    // Create progress dialog
    QProgressDialog progress(tr("Scanning mods..."), tr("Cancel"), 0, 100, mDialog);
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(0);
    progress.setValue(0);

    bool cancelled = false;

    // Perform the rescan
    FomodRescan rescan(mOrganizer, mPatchFinder->mFomodDb.get());
    auto result = rescan.scanAllModsWithChoices([&](int current, int total, const QString& modName) {
        if (progress.wasCanceled()) {
            cancelled = true;
            return;
        }
        const int percent = total > 0 ? (current * 100 / total) : 0;
        progress.setValue(percent);
        progress.setLabelText(tr("Scanning: %1 (%2/%3)").arg(modName).arg(current).arg(total));
        QApplication::processEvents();
    });

    progress.setValue(100);

    if (cancelled) {
        QMessageBox::information(
            mDialog, tr("Rescan Cancelled"), tr("The rescan was cancelled. Partial results may have been saved."));
        logMessage(INFO, "Rescan cancelled by user");
    } else {
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

    // Refresh available patches
    mPatchFinder->populateInstalledPlugins();
    mAvailablePatches = mPatchFinder->getAvailablePatchesForModList();
    logMessage(DEBUG, "Available Patches after rescan: " + std::to_string(mAvailablePatches.size()));

    // Refresh the UI
    display();
}
