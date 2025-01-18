#include "FomodInstallerWindow.h"
#include <log.h>
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
#include <QSizePolicy>
#include <QTextEdit>
#include <QVBoxLayout>
#include <utility>

#include "ui/FomodViewModel.h"

/**
 * 
 * @param installer 
 * @param modName 
 * @param tree
 * @param fomodPath
 * @param viewModel
 * @param parent
 */
FomodInstallerWindow::FomodInstallerWindow(
  FomodPlusInstaller *installer,
  GuessedValue<QString> &modName,
  const std::shared_ptr<IFileTree> &tree,
  QString fomodPath,
  const std::shared_ptr<FomodViewModel> &viewModel,
  QWidget *parent): QDialog(parent),
                    mInstaller(installer),
                    mFomodPath(std::move(fomodPath)),
                    mModName(modName),
                    mTree(tree),
                    mViewModel(viewModel) {
  setupUi();


  mInstallStepStack = new QStackedWidget(this);
  updateInstallStepStack();

  const auto containerLayout = createContainerLayout();
  setLayout(containerLayout);

  updateButtons();
}

void FomodInstallerWindow::onNextClicked() {
  if (!mViewModel->isLastVisibleStep()) {
    mViewModel->stepForward();
    mInstallStepStack->setCurrentIndex(mViewModel->getCurrentStepIndex());
    updateButtons();
    updateDisplayForActivePlugin();
  } else {
    onInstallClicked();
  }
}

void FomodInstallerWindow::onPluginToggled(const bool selected, const std::shared_ptr<GroupViewModel> &group,
                                           const std::shared_ptr<PluginViewModel> &plugin) const {
  std::cout << "Selected? " << selected << std::endl;
  mViewModel->togglePlugin(group, plugin, selected);
  updateButtons();
}

void FomodInstallerWindow::onPluginHovered(const std::shared_ptr<PluginViewModel> &plugin) const {
  mViewModel->setActivePlugin(plugin);
  updateDisplayForActivePlugin();
}

void FomodInstallerWindow::onBackClicked() const {
  mViewModel->stepBack();
  mInstallStepStack->setCurrentIndex(mViewModel->getCurrentStepIndex());
  updateButtons();
  updateDisplayForActivePlugin();
}

void FomodInstallerWindow::onInstallClicked() {
  mModName.update(mModNameInput->currentText(), GUESS_USER);
  mViewModel->preinstall(mTree, mFomodPath);
  // now the installer is available in the outer scope
  // the outer scope should call getFileInstaller() and install there.
  this->accept();
}

void FomodInstallerWindow::updateButtons() const {
  if (mViewModel->getCurrentStepIndex() == 0) {
    mBackButton->setEnabled(false);
  } else {
    mBackButton->setEnabled(true);
  }

  if (mViewModel->isLastVisibleStep()) {
    mNextInstallButton->setText("Install");
  } else {
    mNextInstallButton->setText("Next");
  }
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
  // Create the widgets for each step. Not sure if we need these as member variables. Try it like this for now.
  for (const auto &steps = mViewModel->getSteps(); const auto &installStep: steps) {
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
QBoxLayout *FomodInstallerWindow::createContainerLayout() {
  const auto layout = new QVBoxLayout(this);

  const auto topRow = createTopRow();
  const auto centerRow = createCenterRow();
  const auto bottomRow = createBottomRow();

  layout->addWidget(topRow);
  layout->addWidget(centerRow, 1); // stretch 1 here so the others are static size
  layout->addWidget(bottomRow);

  // NOTE: Disable after debug done
  // UIHelper::setDebugBorders(this);
  return layout;
}

QWidget *FomodInstallerWindow::createCenterRow() {
  const auto centerRow = new QWidget(this);
  auto *centerMainLayout = new QHBoxLayout(centerRow);

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

QWidget *FomodInstallerWindow::createTopRow() {
  const auto topRow = new QWidget(this);

  auto *mainHLayout = new QHBoxLayout(topRow);

  // Holds the name (label), author, version, and website
  auto *metadataLayout = new QHBoxLayout();

  // left side metadata. just the titles of the metadata
  auto *labelsColumn = new QVBoxLayout();
  QLabel *nameLabel = UIHelper::createLabel("Name:", topRow);
  QLabel *authorLabel = UIHelper::createLabel("Author:", topRow);
  QLabel *versionLabel = UIHelper::createLabel("Version:", topRow);
  QLabel *websiteLabel = UIHelper::createLabel("Website:", topRow);

  labelsColumn->addWidget(nameLabel);
  labelsColumn->addWidget(authorLabel);
  labelsColumn->addWidget(versionLabel);
  labelsColumn->addWidget(websiteLabel);

  UIHelper::reduceLabelPadding(labelsColumn);
  UIHelper::setGlobalAlignment(labelsColumn, Qt::AlignTop);

  // the values of the metadata MINUS the search box
  auto *valuesColumn = new QVBoxLayout();
  QLabel *emptyLabel = UIHelper::createLabel("", topRow);
  QLabel *authorValueLabel = UIHelper::createLabel(mViewModel->getInfoViewModel().getAuthor().c_str(), topRow);
  QLabel *versionValueLabel = UIHelper::createLabel(mViewModel->getInfoViewModel().getVersion().c_str(), topRow);
  QLabel *websiteValueLabel = UIHelper::createHyperlink(mViewModel->getInfoViewModel().getWebsite().c_str(), topRow);

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
  auto *modNameComboBox = createModNameComboBox();
  mainHLayout->addWidget(modNameComboBox);
  UIHelper::setGlobalAlignment(mainHLayout, Qt::AlignTop);

  // Extra stuff
  topRow->setLayout(mainHLayout);
  topRow->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
  return topRow;
}

QComboBox *FomodInstallerWindow::createModNameComboBox() {
  mModNameInput = new QComboBox(this);
  mModNameInput->setEditable(true);

  // TODO: Pick the proper guess based on quality instead and sort by quality?
  // std::vector variants(mModName.variants().begin(), mModName.variants().end());
  // ranges::reverse(variants);

  for (const auto &variant: mModName.variants()) {
    mModNameInput->addItem(variant);
  }
  mModNameInput->completer()->setCaseSensitivity(Qt::CaseSensitive);
  return mModNameInput;
}

QWidget *FomodInstallerWindow::createBottomRow() {
  // In vanilla FOMOD installer, left has the Manual button, right has back, next/install, and cancel buttons
  const auto bottomRow = new QWidget(this);
  auto *layout = new QHBoxLayout(bottomRow);

  // Manual on far left
  mManualButton = UIHelper::createButton("Manual", bottomRow);
  layout->addWidget(mManualButton);

  // Space to push remaining buttons right
  layout->addStretch();

  mBackButton = UIHelper::createButton("Back", bottomRow);
  mNextInstallButton = UIHelper::createButton("Next", bottomRow);
  mCancelButton = UIHelper::createButton("Cancel", bottomRow);

  connect(mManualButton, SIGNAL(clicked()), this, SLOT(onManualClicked()));
  connect(mNextInstallButton, SIGNAL(clicked()), this, SLOT(onNextClicked()));
  connect(mBackButton, SIGNAL(clicked()), this, SLOT(onBackClicked()));
  connect(mCancelButton, SIGNAL(clicked()), this, SLOT(reject()));

  layout->addWidget(mBackButton);
  layout->addWidget(mNextInstallButton);
  layout->addWidget(mCancelButton);

  bottomRow->setLayout(layout);
  return bottomRow;
}

QWidget *FomodInstallerWindow::createLeftPane() {
  const auto leftPane = new QWidget(this);
  auto *layout = new QVBoxLayout(leftPane);

  // Add description box
  // Initialize with defaults (the first plugin's description (which defaults to the module image otherwise))
  mDescriptionBox = new QTextEdit("", leftPane);
  layout->addWidget(mDescriptionBox);

  // Add image
  // Initialize with defaults (the first plugin's image)
  mImageLabel = new ScaleLabel(leftPane);
  mImageLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  layout->addWidget(mImageLabel);

  updateDisplayForActivePlugin();

  return leftPane;
}

QWidget *FomodInstallerWindow::createRightPane() {
  const auto rightPane = new QWidget(this);
  auto *layout = new QVBoxLayout(rightPane);

  layout->addWidget(mInstallStepStack);

  return rightPane;
}

QWidget *FomodInstallerWindow::createStepWidget(const std::shared_ptr<StepViewModel> &installStep) {
  const auto stepBox = new QGroupBox(QString::fromStdString(installStep->getName()), this);
  const auto stepBoxLayout = new QVBoxLayout(stepBox);

  auto *scrollArea = new QScrollArea(stepBox);
  scrollArea->setWidgetResizable(true);

  const auto scrollAreaContent = new QWidget(scrollArea);
  auto *scrollAreaLayout = new QVBoxLayout(scrollAreaContent);

  for (const auto &group: installStep->getGroups()) {
    const auto groupSection = renderGroup(group);
    scrollAreaLayout->addWidget(groupSection);
  }

  scrollAreaContent->setLayout(scrollAreaLayout);
  scrollArea->setWidget(scrollAreaContent);

  stepBoxLayout->addWidget(scrollArea);
  stepBox->setLayout(stepBoxLayout);
  return stepBox;
}

QWidget *FomodInstallerWindow::renderGroup(const std::shared_ptr<GroupViewModel> &group) {
  const auto groupBox = new QGroupBox(QString::fromStdString(group->getName()), this);
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
      // renderRadioGroup(groupBox, groupBoxLayout, group);
      break;
    default: ;
  }

  groupBox->setLayout(groupBoxLayout);
  return groupBox;
}

QRadioButton *FomodInstallerWindow::createPluginRadioButton(const std::shared_ptr<PluginViewModel> &plugin,
                                                            const std::shared_ptr<GroupViewModel> &group,
                                                            QWidget *parent) {
  auto *radioButton = new QRadioButton(QString::fromStdString(plugin->getName()), parent);
  auto *hoverFilter = new HoverEventFilter(plugin, this);
  radioButton->installEventFilter(hoverFilter);
  connect(hoverFilter, &HoverEventFilter::hovered, this, &FomodInstallerWindow::onPluginHovered);

  radioButton->setEnabled(plugin->isEnabled());
  radioButton->setChecked(plugin->isSelected());
  // Bind to model function
  connect(radioButton, &QRadioButton::toggled, this, [this, group, plugin](const bool checked) {
    onPluginToggled(checked, group, plugin);
  });

  return radioButton;
}

QCheckBox *FomodInstallerWindow::createPluginCheckBox(const std::shared_ptr<PluginViewModel> &plugin,
                                                      const std::shared_ptr<GroupViewModel> &group, QWidget *parent) {
  auto *checkBox = new QCheckBox(QString::fromStdString(plugin->getName()), parent);

  // Make the hover stuff work
  auto *hoverFilter = new HoverEventFilter(plugin, this);
  checkBox->installEventFilter(hoverFilter);
  connect(hoverFilter, &HoverEventFilter::hovered, this, &FomodInstallerWindow::onPluginHovered);

  checkBox->setEnabled(plugin->isEnabled());
  checkBox->setChecked(plugin->isSelected());
  connect(checkBox, &QCheckBox::stateChanged, this, [this, group, plugin](const int state) {
    onPluginToggled(state == Qt::Checked, group, plugin);
  });
  return checkBox;
}

void FomodInstallerWindow::renderSelectExactlyOne(QWidget *parent, QLayout *parentLayout,
                                               const std::shared_ptr<GroupViewModel> &group) {

  // This is for parity with the legacy installer. Both styles are functionally equivalent
  // for a group size of 1, but they chose checkbox.
  if (group->getPlugins().size() == 1) {
    renderCheckboxGroup(parent, parentLayout, group);
  } else {
    renderRadioGroup(parent, parentLayout, group);
  }
}

void FomodInstallerWindow::renderCheckboxGroup(QWidget *parent, QLayout *parentLayout,
                                               const std::shared_ptr<GroupViewModel> &group) {
  // Same thing but with an extra 'None' option at the end
  // auto* buttonGroup = new QButtonGroup(parent);
  for (const auto &plugin: group->getPlugins()) {
    auto *checkbox = createPluginCheckBox(plugin, group, parent);
    parentLayout->addWidget(checkbox);
  }
}

QButtonGroup *FomodInstallerWindow::renderRadioGroup(QWidget *parent, QLayout *parentLayout,
                                                     const std::shared_ptr<GroupViewModel> &group) {
  auto *buttonGroup = new QButtonGroup(parent);
  buttonGroup->setExclusive(true); // Ensure only one button can be selected

  for (const auto &plugin: group->getPlugins()) {
    auto *radioButton = createPluginRadioButton(plugin, group, parent);
    buttonGroup->addButton(radioButton);
    parentLayout->addWidget(radioButton);
  }
  return buttonGroup;
}


// Updates the image and description field for a given plugin. Also use this on initialization of those widgets.
void FomodInstallerWindow::updateDisplayForActivePlugin() const {
  const auto &plugin = mViewModel->getActivePlugin();
  mDescriptionBox->setText(QString::fromStdString(plugin->getDescription()));
  const auto image = mViewModel->getDisplayImage();
  const auto imagePath = UIHelper::getFullImagePath(mFomodPath, QString::fromStdString(image));
  if (image.empty()) {
    return;
  }
  mImageLabel->setScalableResource(imagePath);
}
