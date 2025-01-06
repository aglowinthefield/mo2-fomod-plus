﻿#include "FomodInstallerWindow.h"
#include "ui/UIHelper.h"
#include "xml/ModuleConfiguration.h"
#include <log.h>
#include "ui/ScaleLabel.h"

#include <QVBoxLayout>
#include <QComboBox>
#include <QCompleter>
#include <QSizePolicy>
#include <QGroupBox>
#include <QTextEdit>
#include <QLabel>

/**
 * 
 * @param installer 
 * @param modName 
 * @param tree
 * @param fomodPath
 * @param fomodFile 
 * @param infoFile 
 * @param parent 
 */
FomodInstallerWindow::FomodInstallerWindow(InstallerFomodPlus *installer, const GuessedValue<QString> &modName,
  const std::shared_ptr<IFileTree> &tree, const QString& fomodPath, std::unique_ptr<ModuleConfiguration> fomodFile,
  std::unique_ptr<FomodInfoFile> infoFile, QWidget *parent): QDialog(parent),
                                                             mInstaller(installer),
                                                             mFomodPath(fomodPath),
                                                             mModName(modName),
                                                             mTree(tree),
                                                             mFomodFile(std::move(fomodFile)),
                                                             mInfoFile(std::move(infoFile)) {

  setupUi();


  mInstallStepStack = new QStackedWidget(this);
  updateInstallStepStack();

  const auto containerLayout = createContainerLayout();
  setLayout(containerLayout);

}

void FomodInstallerWindow::onNextClicked() {
  if (mCurrentStepIndex < mInstallStepStack->count() - 1) {
    mCurrentStepIndex++;
    mInstallStepStack->setCurrentIndex(mCurrentStepIndex);
    updateButtons();
  }
}

void FomodInstallerWindow::onBackClicked() {
  if (mCurrentStepIndex > 0) {
    mCurrentStepIndex--;
    mInstallStepStack->setCurrentIndex(mCurrentStepIndex);
    updateButtons();
  }
}

void FomodInstallerWindow::updateButtons() {
}

void FomodInstallerWindow::setupUi() {
  setMinimumSize(UiConstants::WINDOW_MIN_WIDTH, UiConstants::WINDOW_MIN_HEIGHT);
  setWindowTitle(mModName);
  setWindowModality(Qt::NonModal); // To allow scrolling modlist without closing the window
}

// mInstallStepStack must be initialized before calling this
void FomodInstallerWindow::updateInstallStepStack() {
  if (!mInstallStepStack) {
    log::error("updateInstallStepStack called with no initialized mInstallStepStack. tf?");
    return;
  }

  auto& steps = mFomodFile->installSteps; // TODO: Evaluate if we want to copy this array instead.
  auto compare = [&steps](const InstallStep &a, const InstallStep &b) {
    return steps.compare(a, b, [](const InstallStep& step) { return step.name; });
  };

  // I'm not sure if any modern FOMODs use anything other than Explicit, but hey, it's there.
  if (steps.order != OrderTypeEnum::Explicit) {
    ranges::sort(steps.installSteps, compare);
  }

  // Create the widgets for each step. Not sure if we need these as member variables. Try it like this for now.
  for (auto installStep : steps.installSteps) {
    mInstallStepStack->addWidget(createStepWidget(installStep));
  }
  mInstallStepStack->setCurrentIndex(mCurrentStepIndex);
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
QBoxLayout *FomodInstallerWindow::createContainerLayout() {

  const auto layout = new QVBoxLayout(this);

  // add top row here.
  const auto topRow = createTopRow();
  layout->addWidget(topRow);

  // middle area takes up the bulk of space. maybe 5x the size of the top row
  const auto centerRow = createCenterRow();
  layout->addWidget(centerRow, 1); // stretch 1 here so the others are static size

  // bottom row
  const auto bottomRow = createBottomRow();
  layout->addWidget(bottomRow);

  // any extra layout setup :)
  // UIHelper::setDebugBorders(this);
  return layout;
}

QWidget* FomodInstallerWindow::createCenterRow() {

  const auto centerRow = new QWidget(this);
  auto* centerMainLayout = new QHBoxLayout(centerRow);

  // add the left pane
  const auto leftPaneLayout = new QVBoxLayout(centerRow);
  const auto leftPane = createLeftPane();
  leftPaneLayout->addWidget(leftPane, 1);

  // add the right pane (to be the anchor for renderStep)
  const auto rightPaneLayout = new QVBoxLayout(centerRow);
  const auto rightPane = createRightPane();
  rightPaneLayout->addWidget(rightPane, 1);

  // add panes to layout
  centerMainLayout->addLayout(leftPaneLayout, 1);
  centerMainLayout->addLayout(rightPaneLayout, 1);

  centerRow->setLayout(centerMainLayout);
  return centerRow;

}

QWidget* FomodInstallerWindow::createTopRow() {

  const auto topRow = new QWidget(this);

  auto* mainHLayout = new QHBoxLayout(topRow);

  /*
   * Holds the name (label), author, version, and website
   */
  auto* metadataLayout = new QHBoxLayout();

  // left side metadata. just the titles of the metadata
  auto* labelsColumn = new QVBoxLayout();
  QLabel* nameLabel         = UIHelper::createLabel("Name:", topRow);
  QLabel* authorLabel       = UIHelper::createLabel("Author:", topRow);
  QLabel* versionLabel      = UIHelper::createLabel("Version:", topRow);
  QLabel* websiteLabel      = UIHelper::createLabel("Website:", topRow);

  labelsColumn->addWidget(nameLabel);
  labelsColumn->addWidget(authorLabel);
  labelsColumn->addWidget(versionLabel);
  labelsColumn->addWidget(websiteLabel);

  UIHelper::reduceLabelPadding(labelsColumn);
  UIHelper::setGlobalAlignment(labelsColumn, Qt::AlignTop);

  // the values of the metadata MINUS the search box
  auto* valuesColumn = new QVBoxLayout();
  QLabel* emptyLabel        = UIHelper::createLabel("", topRow);
  QLabel* authorValueLabel  = UIHelper::createLabel(mInfoFile->getAuthor().c_str(), topRow);
  QLabel* versionValueLabel = UIHelper::createLabel(mInfoFile->getVersion().c_str(), topRow);
  QLabel* websiteValueLabel = UIHelper::createHyperlink(mInfoFile->getWebsite().c_str(), topRow);

  valuesColumn->addWidget(emptyLabel);
  valuesColumn->addWidget(authorValueLabel);
  valuesColumn->addWidget(versionValueLabel);
  valuesColumn->addWidget(websiteValueLabel);

  // We want these cleanup fns to be at the layout level directly containing the labels.
  // Since we aren't recursing down the UI forever we can't just call it for mainHLayout.
  UIHelper::reduceLabelPadding(valuesColumn);
  UIHelper::setGlobalAlignment(valuesColumn, Qt::AlignTop);

  metadataLayout->addLayout(labelsColumn);
  metadataLayout->addLayout(valuesColumn);

  mainHLayout->addLayout(metadataLayout);

  // Now make the search bar layout
  auto* modNameComboBox = createModNameComboBox();
  mainHLayout->addWidget(modNameComboBox);
  UIHelper::setGlobalAlignment(mainHLayout, Qt::AlignTop);

  // Extra stuff
  topRow->setLayout(mainHLayout);
  topRow->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
  return topRow;
}

QComboBox* FomodInstallerWindow::createModNameComboBox() {
  auto* modNameComboBox = new QComboBox(this);
  modNameComboBox->setEditable(true);

  // To show the 'best' guess first, we reverse the variant order
  std::vector variants(mModName.variants().begin(), mModName.variants().end());
  ranges::reverse(variants);

  for (const auto& variant : variants) {
    modNameComboBox->addItem(variant);
  }
  modNameComboBox->completer()->setCaseSensitivity(Qt::CaseSensitive);
  return modNameComboBox;
}

QWidget* FomodInstallerWindow::createBottomRow() {
  // In vanilla FOMOD installer, left has the Manual button, right has back, next/install, and cancel buttons
  const auto bottomRow = new QWidget(this);
  auto* layout = new QHBoxLayout(bottomRow);

  // Manual on far left
  mManualButton = UIHelper::createButton("Manual", bottomRow);
  layout->addWidget(mManualButton);

  // Space to push remaining buttons right
  layout->addStretch();

  mBackButton        = UIHelper::createButton("Back", bottomRow);
  mNextInstallButton = UIHelper::createButton("Next", bottomRow);
  mCancelButton      = UIHelper::createButton("Cancel", bottomRow);

  // TODO: Connect the others.
  connect(mCancelButton, SIGNAL(clicked()), this, SLOT(reject()));

  layout->addWidget(mBackButton);
  layout->addWidget(mNextInstallButton);
  layout->addWidget(mCancelButton);

  bottomRow->setLayout(layout);
  return bottomRow;
}

QWidget* FomodInstallerWindow::createLeftPane() {
  const auto leftPane = new QWidget(this);
  auto* layout = new QVBoxLayout(leftPane);

  const auto firstPlugin = mFomodFile->getFirstPlugin();

  const auto defaultText = QString::fromStdString(firstPlugin.description);

  // Add description box
  // Initialize with defaults (the first plugin's description (which defaults to the module image otherwise))
  const auto descriptionBox = new QTextEdit(defaultText, leftPane);
  layout->addWidget(descriptionBox);

  // Add image
  // Initialize with defaults (the first plugin's image)
  const auto imageLabel = new ScaleLabel(leftPane);
  imageLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

  const auto image = mFomodFile->getImageForPlugin(firstPlugin);
  const auto imagePath = UIHelper::getFullImagePath(mFomodPath, image);
  imageLabel->setScalableResource(imagePath);
  layout->addWidget(imageLabel);

  return leftPane;
}

QWidget* FomodInstallerWindow::createRightPane() {
  const auto rightPane = new QWidget(this);
  auto* layout = new QVBoxLayout(rightPane);

  layout->addWidget(mInstallStepStack);

  return rightPane;
}

QWidget* FomodInstallerWindow::createStepWidget(const InstallStep& installStep) {
  // QGroupBox
  const QString stepName = QString::fromStdString(installStep.name);
  const auto stepBox = new QGroupBox(stepName, this);

  const auto stepBoxLayout = new QVBoxLayout(stepBox);

  for (auto group : installStep.optionalFileGroups.groups) {
    const auto groupSection = renderGroup(group);
    stepBoxLayout->addWidget(groupSection);
  }

  stepBox->setLayout(stepBoxLayout);
  return stepBox;
}

QWidget* FomodInstallerWindow::renderGroup(Group&) {
  return nullptr;
}

QWidget* FomodInstallerWindow::renderPlugin(Plugin&) {
  return nullptr;
}
