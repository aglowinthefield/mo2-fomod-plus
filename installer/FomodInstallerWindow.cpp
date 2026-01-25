#include "FomodInstallerWindow.h"

#include "ui/FomodImageViewer.h"

#include "ui/ScaleLabel.h"
#include "ui/UIHelper.h"
#include "xml/ModuleConfiguration.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QCompleter>
#include <QGroupBox>
#include <QLabel>
#include <QRadioButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QSettings>
#include <QSizePolicy>
#include <QSplitter>
#include <QVBoxLayout>
#include <utility>

#include "ui/FomodViewModel.h"

#include <unordered_set>

/**
 * 
 * @param installer 
 * @param modName 
 * @param tree
 * @param fomodPath
 * @param viewModel
 * @param fomodJson
 * @param parent
 */
FomodInstallerWindow::FomodInstallerWindow(
    FomodPlusInstaller* installer,
    GuessedValue<QString>& modName,
    const std::shared_ptr<IFileTree>& tree,
    QString fomodPath,
    const std::shared_ptr<FomodViewModel>& viewModel,
    const nlohmann::json& fomodJson,
    QWidget* parent): QDialog(parent),
                      mInstaller(installer),
                      mFomodPath(std::move(fomodPath)),
                      mModName(modName),
                      mTree(tree),
                      mViewModel(viewModel),
                      mFomodJson(fomodJson)
{
    setupUi();

    mInstallStepStack = new QStackedWidget(this);
    
    // Handle legacy FOMODs with no steps
    if (mViewModel->getSteps().empty()) {
        // Create a simple "Install" widget for legacy FOMODs
        auto* legacyWidget = new QWidget(this);
        auto* layout = new QVBoxLayout(legacyWidget);
        auto* label = new QLabel("This mod will install all files automatically.", legacyWidget);
        layout->addWidget(label);
        mInstallStepStack->addWidget(legacyWidget);
    } else {
        updateInstallStepStack();
        stylePreviouslySelectedOptions();
        stylePreviouslyDeselectedOptions();
    }

    const auto containerLayout = createContainerLayout();
    setLayout(containerLayout);

    updateButtons();
    restoreGeometryAndState();
    
    if (!mViewModel->getSteps().empty()) {
        populatePluginMap();
        if (mInstaller->shouldAutoRestoreChoices()) {
            onSelectPreviousClicked();
        }
    } else {
        // For empty FOMODs, set default description
        mDescriptionBox->setText("This mod will install all files automatically.");
    }
}

void FomodInstallerWindow::closeEvent(QCloseEvent* event)
{
    saveGeometryAndState();
    QDialog::closeEvent(event);
}

void FomodInstallerWindow::saveGeometryAndState() const
{
    const auto cwd = QDir::currentPath();
    QSettings settings(cwd + "/fomod-plus-settings.ini", QSettings::IniFormat);
    settings.setValue("windowGeometry", saveGeometry());
    settings.setValue("centerSplitState", mCenterRow->saveState());
    settings.setValue("leftSplitState", mLeftPane->saveState());
}

void FomodInstallerWindow::restoreGeometryAndState()
{
    const auto cwd = QDir::currentPath();
    const QSettings settings(cwd + "/fomod-plus-settings.ini", QSettings::IniFormat);
    restoreGeometry(settings.value("windowGeometry").toByteArray());
    mCenterRow->restoreState(settings.value("centerSplitState").toByteArray());
    mLeftPane->restoreState(settings.value("leftSplitState").toByteArray());
}

void FomodInstallerWindow::populatePluginMap()
{
    const auto checkboxes   = findChildren<QCheckBox*>();
    const auto radioButtons = findChildren<QRadioButton*>();

    for (const auto& step : mViewModel->getSteps()) {
        for (const auto& group : step->getGroups()) {
            for (const auto& plugin : group->getPlugins()) {

                const auto name = createObjectName(plugin, group);

                for (auto* checkbox : checkboxes) {
                    if (checkbox->objectName() == name) {
                        mPluginMap.insert({ name, { plugin, checkbox } });
                    }
                }
                for (auto* radioButton : radioButtons) {
                    if (radioButton->objectName() == name) {
                        mPluginMap.insert({ name, { plugin, radioButton } });
                    }
                }
            }
        }
    }
}

void FomodInstallerWindow::onNextClicked()
{
    // For legacy FOMODs with no steps, always install
    if (mViewModel->getSteps().empty()) {
        onInstallClicked();
        return;
    }
    
    if (!mViewModel->isLastVisibleStep()) {
        mViewModel->stepForward();
        mInstallStepStack->setCurrentIndex(mViewModel->getCurrentStepIndex());
        updateButtons();
        updateDisplayForActivePlugin();
    } else {
        onInstallClicked();
    }
}

void FomodInstallerWindow::updateCheckboxStates() const
{
    // PluginMap presumed to be populated
    for (const auto& [objectName, pluginData] : mPluginMap) {
        if (pluginData.plugin->isSelected() != pluginData.uiElement->isChecked()) {
            const auto widgetType = pluginData.uiElement->metaObject()->className();
            if (objectName != nullptr) {
                logMessage(DEBUG,
                    "Updating " + objectName.toStdString() + " to state: " + (pluginData.plugin->isSelected()
                        ? "TRUE"
                        : "FALSE") + " because " + widgetType +
                    " selection state is " + (pluginData.uiElement->isChecked() ? "TRUE" : "FALSE"));
            }
            pluginData.uiElement->setChecked(pluginData.plugin->isSelected());
        }

        if (pluginData.plugin->isEnabled() != pluginData.uiElement->isEnabled()) {
            logMessage(DEBUG, "[WINDOW] Changing enabled state of  element " + objectName.toStdString() + " to " +
                (pluginData.plugin->isEnabled() ? "TRUE" : "FALSE"));
            pluginData.uiElement->setEnabled(pluginData.plugin->isEnabled());
        }
    }
}

void FomodInstallerWindow::onPluginToggled(const bool selected, const std::shared_ptr<GroupViewModel>& group,
    const std::shared_ptr<PluginViewModel>& plugin) const
{
    logMessage(INFO,
        "onPluginToggled called with " + plugin->getName() + " in " + group->getName() + ": " +
        std::to_string(selected));
    if (mViewModel->togglePlugin(group, plugin, selected)) {
        updateCheckboxStates();
    }
    if (mNextInstallButton != nullptr) {
        updateButtons();
    }
}

void FomodInstallerWindow::onPluginManuallyUnchecked(const std::shared_ptr<PluginViewModel>& plugin) const
{
    logMessage(INFO, "onPluginManuallyUnchecked called with " + plugin->getName());
    mViewModel->markManuallySet(plugin);
}

void FomodInstallerWindow::onPluginHovered(const std::shared_ptr<PluginViewModel>& plugin) const
{
    mViewModel->setActivePlugin(plugin);
    updateDisplayForActivePlugin();
}

void FomodInstallerWindow::onBackClicked() const
{
    mViewModel->stepBack();
    mInstallStepStack->setCurrentIndex(mViewModel->getCurrentStepIndex());
    updateButtons();
    updateDisplayForActivePlugin();
}

void FomodInstallerWindow::onInstallClicked()
{
    saveGeometryAndState();

    logMessage(DEBUG, "Installing mod: " + mModName->toStdString());
    mModName.update(mModNameInput->currentText(), GUESS_USER);
    mViewModel->preinstall(mTree, mFomodPath);
    // now the installer is available in the outer scope
    // the outer scope should call getFileInstaller() and install there.
    this->accept();
}

void FomodInstallerWindow::updateButtons() const
{
    // For legacy FOMODs with no steps, always show Install
    if (mViewModel->getSteps().empty()) {
        mBackButton->setEnabled(false);
        mNextInstallButton->setText(tr("Install"));
        return;
    }
    
    if (mViewModel->isFirstVisibleStep()) {
        mBackButton->setEnabled(false);
    } else {
        mBackButton->setEnabled(true);
    }

    if (mViewModel->isLastVisibleStep()) {
        mNextInstallButton->setText(tr("Install"));
    } else {
        mNextInstallButton->setText(tr("Next"));
    }
}

void FomodInstallerWindow::setupUi()
{
    setWindowIcon(QIcon(":/fomod/hat"));
    setWindowFlags(Qt::Window); // Allows OS-controlled resizing, including snapping
    setMinimumSize(UiConstants::WINDOW_MIN_WIDTH, UiConstants::WINDOW_MIN_HEIGHT);
    setWindowTitle(mModName);
    setWindowModality(Qt::NonModal); // To allow scrolling modlist without closing the window
}

// mInstallStepStack must be initialized before calling this
void FomodInstallerWindow::updateInstallStepStack()
{
    if (!mInstallStepStack) {
        logMessage(ERR, "updateInstallStepStack called with no initialized mInstallStepStack. tf?");
        return;
    }
    // Create the widgets for each step. Not sure if we need these as member variables. Try it like this for now.
    for (const auto& steps = mViewModel->getSteps(); const auto& installStep : steps) {
        mInstallStepStack->addWidget(createStepWidget(installStep));
    }
    mInstallStepStack->setCurrentIndex(mViewModel->getCurrentStepIndex());
}

/*
+-------------------------------------------------------------------+
| +----------------------------------------------------------------+|
| |                                                                ||
| |                    Metadata and Name Input                     ||
| |                                                                ||
| +----------------------------------------------------------------+|
| +------------------------------++--------------------------------+|
| |                              ||                                ||
| |                              ||                                ||
| |         Description          ||                                ||
| |                              ||                                ||
| |                              ||        Step/Group/Plugins      ||
| |                              ||                                ||
| +------------------------------+|                                ||
| +------------------------------+|                                ||
| |                              ||                                ||
| |                              ||                                ||
| |            Image             ||                                ||
| |                              ||                                ||
| |                              ||                                ||
| |                              ||                                ||
| +----------------------------------------------------------------+|
| |                                                                ||
| |                          Bottom Bar                            ||
| +----------------------------------------------------------------+|
+-------------------------------------------------------------------+
*/
QBoxLayout* FomodInstallerWindow::createContainerLayout()
{
    const auto layout = new QVBoxLayout(this);

    mTopRow    = createTopRow();
    mCenterRow = createCenterRow();
    mBottomRow = createBottomRow();

    layout->addWidget(mTopRow);
    layout->addWidget(mCenterRow, 1); // stretch 1 here so the others are static size
    layout->addWidget(mBottomRow);

    if (mInstaller->shouldShowNotifications()) {
        mNotificationsPanel = createNotificationPanel();
        layout->addWidget(mNotificationsPanel);
        // Set a default welcome message
        addNotification("FOMOD Plus notification panel initialized :)", INFO);
    }
    return layout;
}

QSplitter* FomodInstallerWindow::createCenterRow()
{
    mLeftPane            = createLeftPane(); // Instance var to persist the geometry
    const auto centerRow = new QSplitter(Qt::Horizontal, this);
    const auto rightPane = createRightPane();
    centerRow->addWidget(mLeftPane);
    centerRow->addWidget(rightPane);
    centerRow->setSizes({ width() / 2, width() / 2 });
    return centerRow;
}

QWidget* FomodInstallerWindow::createTopRow()
{
    const auto topRow = new QWidget(this);

    auto* mainHLayout = new QHBoxLayout(topRow);

    // Holds the name (label), author, version, and website
    auto* metadataLayout = new QHBoxLayout();

    // left side metadata. just the titles of the metadata
    auto* labelsColumn   = new QVBoxLayout();
    QLabel* nameLabel    = UIHelper::createLabel(tr("Name:"), topRow);
    QLabel* authorLabel  = UIHelper::createLabel(tr("Author:"), topRow);
    QLabel* versionLabel = UIHelper::createLabel(tr("Version:"), topRow);
    QLabel* websiteLabel = UIHelper::createLabel(tr("Website:"), topRow);

    labelsColumn->addWidget(nameLabel);
    labelsColumn->addWidget(authorLabel);
    labelsColumn->addWidget(versionLabel);
    labelsColumn->addWidget(websiteLabel);

    UIHelper::reduceLabelPadding(labelsColumn);
    UIHelper::setGlobalAlignment(labelsColumn, Qt::AlignTop);

    // the values of the metadata MINUS the search box
    auto* valuesColumn        = new QVBoxLayout();
    QLabel* emptyLabel        = UIHelper::createLabel("", topRow);
    QLabel* authorValueLabel  = UIHelper::createLabel(mViewModel->getInfoViewModel()->getAuthor().c_str(), topRow);
    QLabel* versionValueLabel = UIHelper::createLabel(mViewModel->getInfoViewModel()->getVersion().c_str(), topRow);
    QLabel* websiteValueLabel = UIHelper::createHyperlink(mViewModel->getInfoViewModel()->getWebsite().c_str(), topRow);

    valuesColumn->addWidget(emptyLabel);
    valuesColumn->addWidget(authorValueLabel);
    valuesColumn->addWidget(versionValueLabel);
    valuesColumn->addWidget(websiteValueLabel);

    // We want these cleanup fns to be at the layout level directly containing the labels.
    // Since we aren't recursing down the UI forever we can't just call it for mainHLayout.
    UIHelper::reduceLabelPadding(valuesColumn);
    UIHelper::setGlobalAlignment(valuesColumn, Qt::AlignTop);

    metadataLayout->addLayout(labelsColumn);
    metadataLayout->addLayout(valuesColumn, 1); // To push the right column close to the edge of the left.

    mainHLayout->addLayout(metadataLayout, 1);

    // Now make the search bar layout
    auto* modNameComboBox = createModNameComboBox();
    mainHLayout->addWidget(modNameComboBox, 4);
    UIHelper::setGlobalAlignment(mainHLayout, Qt::AlignTop);

    // Extra stuff
    topRow->setLayout(mainHLayout);
    topRow->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    return topRow;
}

QComboBox* FomodInstallerWindow::createModNameComboBox()
{
    mModNameInput = new QComboBox(this);
    mModNameInput->setEditable(true);

    mModNameInput->addItem(mModName);

    for (const auto& variant : mModName.variants()) {
        if (variant.toStdString() != mModName->toStdString()) {
            mModNameInput->addItem(variant);
        }
    }
    mModNameInput->completer()->setCaseSensitivity(Qt::CaseSensitive);
    return mModNameInput;
}

QWidget* FomodInstallerWindow::createBottomRow()
{
    // In vanilla FOMOD installer, left has the Manual button, right has back, next/install, and cancel buttons
    const auto bottomRow = new QWidget(this);
    auto* layout         = new QHBoxLayout(bottomRow);

    // Manual on far left
    mManualButton         = UIHelper::createButton(tr("Manual"), bottomRow);
    mSelectPreviousButton = UIHelper::createButton(tr("Restore Previous Choices"), bottomRow);
    mResetChoicesButton   = UIHelper::createButton(tr("Reset Choices"), bottomRow);

    layout->addWidget(mManualButton);
    layout->addWidget(mSelectPreviousButton);
    layout->addWidget(mResetChoicesButton);

    // Space to push remaining buttons right
    layout->addStretch();

    mBackButton        = UIHelper::createButton(tr("Back"), bottomRow);
    mNextInstallButton = UIHelper::createButton(tr("Next"), bottomRow);
    mCancelButton      = UIHelper::createButton(tr("Cancel"), bottomRow);

    mNextInstallButton->setDefault(true);
    mNextInstallButton->setAutoDefault(true);

    connect(mManualButton, SIGNAL(clicked()), this, SLOT(onManualClicked()));
    connect(mNextInstallButton, SIGNAL(clicked()), this, SLOT(onNextClicked()));
    connect(mBackButton, SIGNAL(clicked()), this, SLOT(onBackClicked()));
    connect(mCancelButton, SIGNAL(clicked()), this, SLOT(onCancelClicked()));
    connect(mSelectPreviousButton, SIGNAL(clicked()), this, SLOT(onSelectPreviousClicked()));
    connect(mResetChoicesButton, SIGNAL(clicked()), this, SLOT(onResetChoicesClicked()));
    // connect(mHideImagesButton, SIGNAL(clicked()), this, SLOT(toggleImagesShown()));

    layout->addWidget(mBackButton);
    layout->addWidget(mNextInstallButton);
    layout->addWidget(mCancelButton);

    bottomRow->setLayout(layout);
    return bottomRow;
}

QSplitter* FomodInstallerWindow::createLeftPane()
{
    const auto leftPane = new QSplitter(Qt::Vertical, this);

    // Add description box
    // Initialize with defaults (the first plugin's description (which defaults to the module image otherwise))
    auto* scrollArea = new QScrollArea(leftPane);
    scrollArea->setWidgetResizable(true);

    mDescriptionBox = new QLabel("", leftPane);
    mDescriptionBox->setTextFormat(Qt::RichText);
    mDescriptionBox->setTextInteractionFlags(Qt::TextBrowserInteraction);
    mDescriptionBox->setOpenExternalLinks(true);
    mDescriptionBox->setWordWrap(true);
    mDescriptionBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    mDescriptionBox->setAlignment(Qt::AlignTop | Qt::AlignLeft);

    scrollArea->setWidget(mDescriptionBox);
    leftPane->addWidget(scrollArea);

    // Add image
    // Initialize with defaults (the first plugin's image)
    mImageLabel = new ScaleLabel(leftPane);
    mImageLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    connect(mImageLabel, &ScaleLabel::clicked, this, [this] {
        if (!mImageLabel->hasResource()) {
            return;
        }
        const auto viewer = new FomodImageViewer(this, mFomodPath, mViewModel->getActiveStep(),
            mViewModel->getActivePlugin());
        viewer->showMaximized();
    });

    if (!mInstaller->shouldShowImages()) {
        mImageLabel->hide();
    }

    leftPane->addWidget(mImageLabel);
    leftPane->setSizes({ height() / 2, height() / 2 });

    updateDisplayForActivePlugin();

    return leftPane;
}

QWidget* FomodInstallerWindow::createRightPane()
{
    const auto rightPane = new QWidget(this);
    auto* layout         = new QVBoxLayout(rightPane);

    layout->addWidget(mInstallStepStack);

    return rightPane;
}

QTextEdit* FomodInstallerWindow::createNotificationPanel()
{
    auto* panel = new QTextEdit(this);
    panel->setReadOnly(true);
    panel->setMaximumHeight(100); // Limit height
    panel->setStyleSheet("font-family: monospace; font-size: 9pt;");

    return panel;
}

QWidget* FomodInstallerWindow::createStepWidget(const std::shared_ptr<StepViewModel>& installStep)
{
    const auto stepBox       = new QGroupBox(QString::fromStdString(installStep->getName()), this);
    const auto stepBoxLayout = new QVBoxLayout(stepBox);

    auto* scrollArea = new QScrollArea(stepBox);
    scrollArea->setWidgetResizable(true);

    const auto scrollAreaContent = new QWidget(scrollArea);
    auto* scrollAreaLayout       = new QVBoxLayout(scrollAreaContent);

    for (const auto& group : installStep->getGroups()) {
        const auto groupSection = renderGroup(group);
        scrollAreaLayout->addWidget(groupSection);
    }

    scrollAreaContent->setLayout(scrollAreaLayout);
    scrollArea->setWidget(scrollAreaContent);

    stepBoxLayout->addWidget(scrollArea);
    stepBox->setLayout(stepBoxLayout);
    return stepBox;
}

QWidget* FomodInstallerWindow::renderGroup(const std::shared_ptr<GroupViewModel>& group)
{
    const auto groupBox       = new QGroupBox(QString::fromStdString(group->getName()), this);
    const auto groupBoxLayout = new QVBoxLayout(groupBox);

    switch (group->getType()) {
    case SelectAtLeastOne:
    case SelectAny:
    case SelectAll:
        renderCheckboxGroup(groupBox, groupBoxLayout, group);
        break;
    case SelectExactlyOne:
    case SelectAtMostOne:
        renderSelectExactlyOne(groupBox, groupBoxLayout, group);
        break;
    default: ;
    }

    groupBox->setLayout(groupBoxLayout);
    return groupBox;
}

QString FomodInstallerWindow::createObjectName(const std::shared_ptr<PluginViewModel>& plugin,
    const std::shared_ptr<GroupViewModel>& group)
{
    const std::string objectName = std::format("[{}:{}] {}-{}",
        group->getStepIndex(), group->getOwnIndex(),
        group->getName(), plugin->getName());

    return QString::fromStdString(objectName);
}

QRadioButton* FomodInstallerWindow::createPluginRadioButton(const std::shared_ptr<PluginViewModel>& plugin,
    const std::shared_ptr<GroupViewModel>& group,
    QWidget* parent)
{
    auto* radioButton = new QRadioButton(QString::fromStdString(plugin->getName()), parent);
    radioButton->setObjectName(createObjectName(plugin, group));
    auto* hoverFilter = new HoverEventFilter(plugin, this);
    radioButton->installEventFilter(hoverFilter);
    connect(hoverFilter, &HoverEventFilter::hovered, this, &FomodInstallerWindow::onPluginHovered);

    connect(radioButton, &QRadioButton::toggled, this, [this, radioButton, group, plugin](const bool checked) {
        logMessage(INFO,
            "Received toggled signal for radio: " + plugin->getName() + ": " + (checked ? "TRUE" : "FALSE") +
            " Radio is now: " + (radioButton->isChecked() ? "TRUE" : "FALSE"));
        onPluginToggled(checked, group, plugin);
    });

    radioButton->setEnabled(plugin->isEnabled());
    radioButton->setChecked(plugin->isSelected());
    return radioButton;
}

QCheckBox* FomodInstallerWindow::createPluginCheckBox(const std::shared_ptr<PluginViewModel>& plugin,
    const std::shared_ptr<GroupViewModel>& group, QWidget* parent)
{
    auto* checkBox = new QCheckBox(QString::fromStdString(plugin->getName()), parent);
    checkBox->setObjectName(createObjectName(plugin, group));

    // Make the hover stuff work
    auto* hoverFilter = new HoverEventFilter(plugin, this);
    checkBox->installEventFilter(hoverFilter);
    connect(hoverFilter, &HoverEventFilter::hovered, this, &FomodInstallerWindow::onPluginHovered);

    // Install Ctrl+click event filter
    auto* ctrlClickFilter = new CtrlClickEventFilter(plugin, group, this);
    checkBox->installEventFilter(ctrlClickFilter);

    checkBox->setEnabled(plugin->isEnabled());
    checkBox->setChecked(plugin->isSelected());
    connect(checkBox, &QCheckBox::clicked, this, [this, plugin](const bool checked) {
        // Send a message to viewModel saying the user deactivated the plugin manually.
        // This may get overridden later by automatic checking, but we'll reconcile that at JSON serialization time.
        if (!checked) {
            onPluginManuallyUnchecked(plugin);
        }
    });
    connect(checkBox, &QCheckBox::toggled, this, [this, checkBox, group, plugin](const bool checked) {
        logMessage(INFO,
            "Received toggled signal for checkbox: " + plugin->getName() + ": " + (checked ? "true" : "false") +
            " Checkbox was previously " + (checkBox->isChecked() ? "true" : "false"));
        onPluginToggled(checked, group, plugin);
    });
    return checkBox;
}

void FomodInstallerWindow::renderSelectExactlyOne(QWidget* parent, QLayout* parentLayout,
    const std::shared_ptr<GroupViewModel>& group)
{
    // This is for parity with the legacy installer. Both styles are functionally equivalent
    // for a group size of 1, but they chose checkbox.
    if (group->getPlugins().size() == 1) {
        renderCheckboxGroup(parent, parentLayout, group);
    } else {
        renderRadioGroup(parent, parentLayout, group);
    }
}

void FomodInstallerWindow::renderCheckboxGroup(QWidget* parent, QLayout* parentLayout,
    const std::shared_ptr<GroupViewModel>& group)
{
    for (const auto& plugin : group->getPlugins()) {
        auto* checkbox = createPluginCheckBox(plugin, group, parent);
        parentLayout->addWidget(checkbox);
    }
}

QButtonGroup* FomodInstallerWindow::renderRadioGroup(QWidget* parent, QLayout* parentLayout,
    const std::shared_ptr<GroupViewModel>& group)
{
    auto* buttonGroup = new QButtonGroup(parent);
    buttonGroup->setExclusive(true); // Ensure only one button can be selected

    for (const auto& plugin : group->getPlugins()) {
        auto* radioButton = createPluginRadioButton(plugin, group, parent);
        buttonGroup->addButton(radioButton);
        parentLayout->addWidget(radioButton);
    }
    return buttonGroup;
}

void FomodInstallerWindow::addNotification(const QString& message, const LogLevel level) const
{
    if (!mNotificationsPanel) {
        return;
    }

    const QString timestamp    = QDateTime::currentDateTime().toString("hh:mm:ss");
    const QString formattedMsg = QString("<span>[%2] [%3] %4</span>")
        .arg(timestamp).arg(logLevelToString(level)).arg(message);

    mNotificationsPanel->append(formattedMsg);

    // Auto-scroll to bottom
    QScrollBar* scrollbar = mNotificationsPanel->verticalScrollBar();
    scrollbar->setValue(scrollbar->maximum());
}

[[deprecated]]
void FomodInstallerWindow::toggleImagesShown() const
{
    logMessage(DEBUG, "Toggling image visibility");
    mInstaller->toggleShouldShowImages();
    if (mInstaller->shouldShowImages()) {
        logMessage(DEBUG, "Turning images ON");
        mImageLabel->show();
        mHideImagesButton->setText(tr("Hide Images"));
    } else {
        logMessage(DEBUG, "Turning images OFF");
        mImageLabel->hide();
        mHideImagesButton->setText(tr("Show Images"));
    }
}


// Updates the image and description field for a given plugin. Also use this on initialization of those widgets.
void FomodInstallerWindow::updateDisplayForActivePlugin() const
{
    // Skip if no steps (legacy FOMOD)
    if (mViewModel->getSteps().empty()) {
        return;
    }

    auto plugin = mViewModel->getActivePlugin();
    if (!plugin) {
        const auto activeStep = mViewModel->getActiveStep();
        if (!activeStep || activeStep->getGroups().empty() || activeStep->getGroups().front()->getPlugins().empty()) {
            mDescriptionBox->setText(tr("Select a plugin to see its description."));
            mImageLabel->clear();
            return;
        }
        // Fall back to the first plugin in the active step when no active plugin is set.
        plugin = activeStep->getGroups().front()->getPlugins().front();
        mViewModel->setActivePlugin(plugin);
    }

    const QString description = formatPluginDescription(QString::fromStdString(plugin->getDescription()));
    mDescriptionBox->setText(description);

    const auto image     = mViewModel->getDisplayImage();
    if (image.empty()) {
        mImageLabel->clear();
        return;
    }

    const auto imagePath = UIHelper::getFullImagePath(mFomodPath, QString::fromStdString(image));
    mImageLabel->setScalableResource(imagePath);
}

/**
 *
 * @param pluginSelector For now either 'plugins', or 'deselected'. The key of the member of 'groups' to iterate over.
 * @param fn The callback for each plugin in the chosen group member.
 */
void FomodInstallerWindow::applyFnFromJson(const std::string& pluginSelector,
    const std::function<void(QAbstractButton*)>& fn)
{
    if (mFomodJson.empty()) {
        return;
    }

    const auto jsonSteps = mFomodJson["steps"];
    // for each step in JSON, create a <group>-<plugin> string out of the { groups: [ { plugins... } ] } array
    vector<std::string> selectedPlugins;

    // TODO: Can groups have the same name within a step, or across steps? How do we account for that?
    for (int stepIndex = 0; stepIndex < jsonSteps.size(); ++stepIndex) {
        const auto& step = jsonSteps[stepIndex];
        for (int groupIndex = 0; groupIndex < step["groups"].size(); ++groupIndex) {
            const auto& group = step["groups"][groupIndex];

            if (!group.contains(pluginSelector)) {
                continue;
            }

            for (int pluginIndex = 0; pluginIndex < group[pluginSelector].size(); ++pluginIndex) {
                const auto& plugin = group[pluginSelector][pluginIndex];

                std::string name = std::format("[{}:{}] {}-{}",
                    stepIndex, groupIndex, group["name"].get<std::string>(), plugin.get<std::string>());
                selectedPlugins.push_back(name);
            }
        }
    }

    const auto checkboxes   = findChildren<QCheckBox*>();
    const auto radioButtons = findChildren<QRadioButton*>();

    for (auto* checkbox : checkboxes) {
        for (const auto& selectedPlugin : selectedPlugins) {
            if (checkbox->objectName().toStdString() == selectedPlugin) {
                fn(checkbox);
            }
        }
    }
    for (auto* radio : radioButtons) {
        for (const auto& selectedPlugin : selectedPlugins) {
            if (radio->objectName().toStdString() == selectedPlugin) {
                fn(radio);
            }
        }
    }
}

void FomodInstallerWindow::stylePreviouslySelectedOptions()
{
    const auto stylesheet = getColorStyle(UiColors::ColorApplication::BACKGROUND);

    const auto tooltip = "You previously selected this plugin when installing this mod.";

    logMessage(INFO, "Styling previously selected choices with stylesheet " + stylesheet.toStdString(), true);
    applyFnFromJson("plugins", [stylesheet, tooltip](QAbstractButton* button) {
        button->setStyleSheet(stylesheet);
        button->setToolTip(tooltip);
    });
}

void FomodInstallerWindow::stylePreviouslyDeselectedOptions()
{
    const auto stylesheet = getColorStyle(UiColors::ColorApplication::BORDER);
    const auto tooltip    = "You previously unchecked this plugin when installing this mod.";
    applyFnFromJson("deselected", [stylesheet, tooltip](QAbstractButton* button) {
        button->setStyleSheet(stylesheet);
        button->setToolTip(tooltip);
    });
}

void FomodInstallerWindow::selectPreviouslySelectedOptions() const
{
    logMessage(INFO, "Selecting previously selected choices", true);
    logMessage(INFO, "Existing JSON provided: " + mFomodJson.dump(4));
    if (mFomodJson.empty()) {
        return;
    }
    try {
        mViewModel->selectFromJson(mFomodJson);
    } catch (Exception& e) {
        logMessage(ERR, std::string("Error selecting previously selected options: ") + e.what(), true);
    }
    updateCheckboxStates();
}

void FomodInstallerWindow::onResetChoicesClicked()
{
    logMessage(INFO, "Resetting choices to author defaults", true);
    try {
        mViewModel->resetToDefaults();
    } catch (Exception& e) {
        logMessage(ERR, std::string("Error resetting choices: ") + e.what(), true);
        return;
    }

    // Reset the UI to show the first step
    mInstallStepStack->setCurrentIndex(mViewModel->getCurrentStepIndex());
    updateCheckboxStates();
    updateButtons();
    updateDisplayForActivePlugin();
}

QString FomodInstallerWindow::getColorStyle(const UiColors::ColorApplication color_application) const
{
    const auto selectedColor = mInstaller->getSelectedColor();
    logMessage(DEBUG, "Selected color: " + selectedColor.toStdString());
    return getStyle(selectedColor, color_application);
}
