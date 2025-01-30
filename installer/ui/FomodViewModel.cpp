#include "FomodViewModel.h"
#include "xml/ModuleConfiguration.h"

/*
--------------------------------------------------------------------------------
                               Lifecycle
--------------------------------------------------------------------------------
*/

/**
 * @brief FomodViewModel constructor
 *
 * @note DO NOT USE DIRECTLY. We should only use FomodViewModel::create() to create a new instance.
 * @see FomodViewModel::create
 *
 * @param organizer The organizer instance passed from the IInstaller
 * @param fomodFile The ModuleConfiguration instance created from the raw ModuleConfiguration.xml file
 * @param infoFile  The FomodInfoFile instance created from the raw info.xml file
 *
 * @return A FomodViewModel instance
 */
FomodViewModel::FomodViewModel(MOBase::IOrganizer* organizer,
    std::unique_ptr<ModuleConfiguration> fomodFile,
    std::unique_ptr<FomodInfoFile> infoFile)
    : mOrganizer(organizer), mFomodFile(std::move(fomodFile)), mInfoFile(std::move(infoFile)),
      mConditionTester(organizer),
      mInfoViewModel(std::make_shared<InfoViewModel>(mInfoFile))
{
    mFlags = std::make_shared<FlagMap>();
}

/**
 *
 * @param organizer The organizer instance passed from the IInstaller
 * @param fomodFile The ModuleConfiguration instance created from the raw ModuleConfiguration.xml file
 * @param infoFile  The FomodInfoFile instance created from the raw info.xml file
 * @return A shared pointer to the FomodViewModel instance
 */
std::shared_ptr<FomodViewModel> FomodViewModel::create(MOBase::IOrganizer* organizer,
    std::unique_ptr<ModuleConfiguration> fomodFile,
    std::unique_ptr<FomodInfoFile> infoFile)
{
    auto viewModel = std::make_shared<FomodViewModel>(organizer, std::move(fomodFile), std::move(infoFile));
    if (viewModel->mFlags == nullptr) {
        viewModel->mFlags = std::make_shared<FlagMap>();
    }
    viewModel->createStepViewModels();
    viewModel->setupGroups();
    viewModel->enforceGroupConstraints();
    viewModel->processPluginConditions();
    viewModel->updateVisibleSteps();
    viewModel->mInitialized  = true;
    viewModel->mActiveStep   = viewModel->mSteps.at(0);
    viewModel->mActivePlugin = viewModel->getFirstPluginForActiveStep();
    std::cout << "Active Plugin: " << viewModel->mActivePlugin->getName() << std::endl;
    return viewModel;
}

/*
--------------------------------------------------------------------------------
                               Initialization
--------------------------------------------------------------------------------
*/

bool isRadioButtonGroup(const GroupTypeEnum groupType)
{
    return groupType == SelectExactlyOne || groupType == SelectAtMostOne;
}

const std::shared_ptr<PluginViewModel>& FomodViewModel::getFirstPluginForActiveStep() const
{
    return mActiveStep->getGroups().at(0)->getPlugins().at(0);
}

void FomodViewModel::setupGroups() const
{
    for (const auto& step : mSteps) {
        for (const auto& group : step->getGroups()) {
            if (group->getType() == SelectAtMostOne && group->getPlugins().size() > 1) {
                createNonePluginForGroup(group);
            }
        }
    }
}

// TODO: Only do this if there is no other plugin that can be selected.
void FomodViewModel::createNonePluginForGroup(const std::shared_ptr<GroupViewModel>& group) const
{
    const auto nonePlugin           = std::make_shared<Plugin>();
    nonePlugin->name                = "None";
    nonePlugin->typeDescriptor.type = PluginTypeEnum::Recommended;
    const auto nonePluginViewModel  = std::make_shared<PluginViewModel>(nonePlugin, false, true);
    group->plugins.emplace_back(nonePluginViewModel);
    togglePlugin(group, nonePluginViewModel, true);
}

void FomodViewModel::processPlugin(const std::shared_ptr<GroupViewModel>& groupViewModel,
    const std::shared_ptr<PluginViewModel>& pluginViewModel) const
{
    const auto typeDescriptor = mConditionTester.getPluginTypeDescriptorState(pluginViewModel->plugin, mFlags);
    if (typeDescriptor == PluginTypeEnum::Recommended) {
        pluginViewModel->setEnabled(true);
        togglePlugin(groupViewModel, pluginViewModel, true);
    }
    if (typeDescriptor == PluginTypeEnum::Required) {
        pluginViewModel->setEnabled(false);
        togglePlugin(groupViewModel, pluginViewModel, true);
    }
    if (typeDescriptor == PluginTypeEnum::NotUsable) {
        pluginViewModel->setEnabled(false);
        togglePlugin(groupViewModel, pluginViewModel, false);
    }
}

void FomodViewModel::enforceGroupConstraints() const
{
    for (const auto& stepViewModel : getSteps()) {
        for (const auto& groupViewModel : stepViewModel->getGroups()) {
            if (!isRadioButtonGroup(groupViewModel->getType())) {
                return; // Nothing to do for other groups constraint-wise...yet. TODO: Figure out if that's true.
            }

            if (std::ranges::any_of(groupViewModel->plugins, [](const auto& plugin) { return plugin->isSelected(); })) {
                return; // We're good if at least one is selected.
            }

            // Select the first plugin that isn't NotUsable
            for (const auto& plugin : groupViewModel->plugins) {
                if (mConditionTester.getPluginTypeDescriptorState(plugin->getPlugin(), mFlags) !=
                    PluginTypeEnum::NotUsable) {
                    togglePlugin(groupViewModel, plugin, true);
                    break;
                }
            }

            // My debugging function was wrong here at first. We really shouldn't see this logged now.
            if (std::ranges::all_of(groupViewModel->plugins, [](const auto& plugin) {
                return !plugin->isSelected();
            })) {
                std::cerr << "SelectExactlyOne had no selectable members! Please debug this." << std::endl;
            }
        }
    }
}

void FomodViewModel::processPluginConditions() const
{
    for (const auto& stepViewModel : mSteps) {
        for (const auto& groupViewModel : stepViewModel->getGroups()) {
            if (isRadioButtonGroup(groupViewModel->getType())) {
                continue;
            }
            for (const auto& pluginViewModel : groupViewModel->getPlugins()) {
                processPlugin(groupViewModel, pluginViewModel);
            }
        }
    }
}

void FomodViewModel::createStepViewModels()
{
    shared_ptr_list<StepViewModel> stepViewModels;

    for (const auto& installStep : mFomodFile->installSteps.installSteps) {
        std::vector<std::shared_ptr<GroupViewModel> > groupViewModels;

        for (const auto& group : installStep.optionalFileGroups.groups) {
            std::vector<std::shared_ptr<PluginViewModel> > pluginViewModels;

            for (const auto& plugin : group.plugins.plugins) {
                auto pluginViewModel = std::make_shared<PluginViewModel>(std::make_shared<Plugin>(plugin), false, true);
                pluginViewModels.emplace_back(pluginViewModel); // Assuming default values for selected and enabled
            }
            auto groupViewModel = std::make_shared<GroupViewModel>(std::make_shared<Group>(group), pluginViewModels);
            groupViewModels.emplace_back(groupViewModel);

        }
        auto stepViewModel = std::make_shared<StepViewModel>(std::make_shared<InstallStep>(installStep),
            std::move(groupViewModels));
        stepViewModels.emplace_back(stepViewModel);
    }
    // TODO Sort the view models here, maybe
    mSteps = std::move(stepViewModels);
}


void FomodViewModel::setFlagForPluginState(const std::shared_ptr<PluginViewModel>& plugin, const bool selected) const
{
    for (const auto& flag : plugin->plugin->conditionFlags.flags) {
        if (flag.name == "EmbersXD" && !selected) {
            std::cerr << "Disabling EmbersXD Flag" << std::endl;

        }
        mFlags->setFlag(flag.name, selected ? flag.value : "");
    }
}

void FomodViewModel::togglePlugin(const std::shared_ptr<GroupViewModel>& group,
    const std::shared_ptr<PluginViewModel>& plugin, const bool selected) const
{
    if (plugin->getImagePath() == "img/new.jpg") {
        std::cout << "Debugging here" << std::endl;
    }

    plugin->setSelected(selected);
    setFlagForPluginState(plugin, selected);

    if (mInitialized) {
        // for radio groups, we need to toggle the other plugins to be off first.
        if (isRadioButtonGroup(group->getType())) {
            for (const auto& otherPlugin : group->getPlugins()) {
                if (otherPlugin != plugin && otherPlugin->isSelected()) {
                    std::cout << "Disabling " << otherPlugin->getName() << " because we're enabling " << plugin->
                        getName() << std::endl;
                    otherPlugin->setSelected(false);
                    setFlagForPluginState(otherPlugin, false);
                }
            }
        }
        mActivePlugin = plugin;
        updateVisibleSteps();
    }
}

void FomodViewModel::updateVisibleSteps() const
{
    mVisibleStepIndices.clear();
    for (int i = 0; i < mSteps.size(); ++i) {
        if (mConditionTester.isStepVisible(mFlags, mSteps[i]->installStep)) {
            mVisibleStepIndices.push_back(i);
        }
    }
}

void FomodViewModel::preinstall(const std::shared_ptr<MOBase::IFileTree>& tree, const QString& fomodPath)
{
    mFileInstaller = std::make_shared<
        FileInstaller>(mOrganizer, fomodPath, tree, std::move(mFomodFile), mFlags, mSteps);
}

bool FomodViewModel::isLastVisibleStep() const
{
    return !mVisibleStepIndices.empty() && mCurrentStepIndex == mVisibleStepIndices.back();
}

/*
--------------------------------------------------------------------------------
                               Navigation
--------------------------------------------------------------------------------
*/
void FomodViewModel::stepBack()
{
    const auto it = std::ranges::find(mVisibleStepIndices, mCurrentStepIndex);
    if (it != mVisibleStepIndices.end() && it != mVisibleStepIndices.begin()) {
        mCurrentStepIndex = *std::prev(it);
        mActiveStep       = mSteps[mCurrentStepIndex];
        mActivePlugin     = getFirstPluginForActiveStep();
    }
}

void FomodViewModel::stepForward()
{
    const auto it = std::ranges::find(mVisibleStepIndices, mCurrentStepIndex);
    if (it != mVisibleStepIndices.end() && std::next(it) != mVisibleStepIndices.end()) {
        mCurrentStepIndex = *std::next(it);
        mActiveStep       = mSteps[mCurrentStepIndex];
        mActivePlugin     = getFirstPluginForActiveStep();
    }
}

/*
--------------------------------------------------------------------------------
                               Display
--------------------------------------------------------------------------------
*/
std::string FomodViewModel::getDisplayImage() const
{
    // if the active plugin has an image, return it
    if (!mActivePlugin->getImagePath().empty()) {
        return mActivePlugin->getImagePath();
    }

    // otherwise show the default fomod image if we're on step 1.
    if (mCurrentStepIndex == 0) {
        return mFomodFile->moduleImage.path;
    }

    // otherwise nothin'
    return "";
}