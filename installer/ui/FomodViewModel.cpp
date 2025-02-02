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

void FomodViewModel::forEachGroup(
    const std::function<void(const std::shared_ptr<GroupViewModel>&)>&
    callback) const
{
    for (const auto& stepViewModel : mSteps) {
        for (const auto& groupViewModel : stepViewModel->getGroups()) {
            callback(groupViewModel);
        }
    }
}

void FomodViewModel::forEachPlugin(
    const std::function<void(const std::shared_ptr<GroupViewModel>&, const std::shared_ptr<PluginViewModel>&)>&
    callback) const
{
    for (const auto& stepViewModel : mSteps) {
        for (const auto& groupViewModel : stepViewModel->getGroups()) {
            for (const auto& pluginViewModel : groupViewModel->getPlugins()) {
                callback(groupViewModel, pluginViewModel);
            }
        }
    }
}

void FomodViewModel::forEachFuturePlugin(
    const std::function<void(const std::shared_ptr<GroupViewModel>&, const std::shared_ptr<PluginViewModel>&)>&
    callback) const
{
    for (int i = 0; i < mSteps.size(); ++i) {
        if (i <= mCurrentStepIndex) {
            continue;
        }
        for (const auto& groupViewModel : mSteps[i]->getGroups()) {
            for (const auto& pluginViewModel : groupViewModel->getPlugins()) {
                callback(groupViewModel, pluginViewModel);
            }
        }
    }
}

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
    forEachGroup([this](const auto& group) {
        if (group->getType() == SelectAtMostOne && group->getPlugins().size() > 1) {
            createNonePluginForGroup(group);
        }
    });
}

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
    // Might be a workaround. lets see.
    if (groupViewModel->getType() == SelectAll) {
        return;
    }
    const auto typeDescriptor = mConditionTester.getPluginTypeDescriptorState(pluginViewModel->plugin, mFlags);
    std::cout << "Processing plugin " << pluginViewModel->getName() << " with type " << typeDescriptor << std::endl;

    const bool isOnlyPlugin = groupViewModel->getPlugins().size() == 1
        && (groupViewModel->getType() == SelectExactlyOne || groupViewModel->getType() == SelectAtLeastOne);

    switch (typeDescriptor) {
    case PluginTypeEnum::Recommended:
        if (!pluginViewModel->isEnabled()) {
            pluginViewModel->setEnabled(true);
        }
        togglePlugin(groupViewModel, pluginViewModel, true);
        break;
    case PluginTypeEnum::Required:
        if (pluginViewModel->isEnabled()) {
            pluginViewModel->setEnabled(false);
        }
        if (!pluginViewModel->isSelected()) {
            togglePlugin(groupViewModel, pluginViewModel, true);
        }
        break;
    case PluginTypeEnum::Optional:
        if (!pluginViewModel->isEnabled() && !isOnlyPlugin) {
            pluginViewModel->setEnabled(true);
        }
        break;
    case PluginTypeEnum::NotUsable:
        if (pluginViewModel->isEnabled()) {
            pluginViewModel->setEnabled(false);
        }
        if (pluginViewModel->isSelected()) {
            togglePlugin(groupViewModel, pluginViewModel, false);
        }
        break;
    case PluginTypeEnum::CouldBeUsable:
        if (!pluginViewModel->isEnabled()) {
            pluginViewModel->setEnabled(true);
        }
        break;
    default: ;
    }
}

void FomodViewModel::enforceRadioGroupConstraints(const std::shared_ptr<GroupViewModel>& groupViewModel) const
{
    if (groupViewModel->getType() != SelectExactlyOne) {
        return;
    }
    // if (!isRadioButtonGroup(groupViewModel->getType())) {
    //     return; // Nothing to do for other groups constraint-wise...yet. TODO: Figure out if that's true.
    // }

    std::cout << "Enforcing group constraints for group " << groupViewModel->getName() << std::endl;

    if (groupViewModel->getType() == SelectExactlyOne && groupViewModel->plugins.size() == 1) {
        std::cout << "Disabling " << groupViewModel->getPlugins().at(0)->getName() << " because it's the only plugin." << std::endl;
        groupViewModel->getPlugins().at(0)->setEnabled(false);
    }

    if (std::ranges::any_of(groupViewModel->plugins, [](const auto& plugin) { return plugin->isSelected(); })) {
        std::cout << "At least one plugin is selected. Nothing to enforce." << std::endl;
        return; // We're good if at least one is selected.
    }

    // First, try to select the first Recommended plugin
    for (const auto& plugin : groupViewModel->plugins) {
        if (mConditionTester.getPluginTypeDescriptorState(plugin->getPlugin(), mFlags) == PluginTypeEnum::Recommended) {
            std::cout << "Selecting " << plugin->getName() << " because it's the first recommended plugin." << std::endl;
            togglePlugin(groupViewModel, plugin, true);
            return;
        }
    }

    // If no Recommended plugin is found, select the first one that isn't NotUsable
    for (const auto& plugin : groupViewModel->plugins) {
        if (mConditionTester.getPluginTypeDescriptorState(plugin->getPlugin(), mFlags) != PluginTypeEnum::NotUsable) {
            std::cout << "Selecting " << plugin->getName() << " because it's the first usable plugin." << std::endl;
            togglePlugin(groupViewModel, plugin, true);
            return;
        }
    }

}

void FomodViewModel::enforceSelectAllConstraint(const std::shared_ptr<GroupViewModel>& groupViewModel) const
{
    if (groupViewModel->getType() != SelectAll) {
        return;
    }

    for (const auto& pluginViewModel : groupViewModel->getPlugins()) {
        togglePlugin(groupViewModel, pluginViewModel, true);
        pluginViewModel->setEnabled(false);
    }

}

void FomodViewModel::enforceSelectAtLeastOneConstraint(const std::shared_ptr<GroupViewModel>& groupViewModel) const
{
    if (groupViewModel->getType() != SelectAtLeastOne) {
        return;
    }

    if (groupViewModel->getPlugins().size() == 1) {
        const auto& plugin = groupViewModel->getPlugins().at(0);
        if (mConditionTester.getPluginTypeDescriptorState(plugin->getPlugin(), mFlags) != PluginTypeEnum::NotUsable) {
            togglePlugin(groupViewModel, plugin, true);
            plugin->setEnabled(false);
        }
    }
}

void FomodViewModel::enforceGroupConstraints() const
{
    forEachGroup([this](const auto& groupViewModel) {
        enforceRadioGroupConstraints(groupViewModel);
        enforceSelectAllConstraint(groupViewModel);
        enforceSelectAtLeastOneConstraint(groupViewModel);
    });
}

void FomodViewModel::processPluginConditions() const
{
    // We only want to update plugins that haven't been seen yet. Otherwise we could undo manual selections by the user.
    if (mInitialized) {
        forEachFuturePlugin([this](const auto& groupViewModel, const auto& pluginViewModel) {
            processPlugin(groupViewModel, pluginViewModel);
        });
    } else {
        forEachPlugin([this](const auto& groupViewModel, const auto& pluginViewModel) {
            processPlugin(groupViewModel, pluginViewModel);
        });
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
        mFlags->setFlag(flag.name, selected ? flag.value : "");
    }
}

/*
 * In an exclusive group, this gets called for the deselected plugin and then the selected plugin.
 * So if we're unselecting modB to select modA, we will get calls like
 *  togglePlugin(group, modB, false)
 *  togglePlugin(group, modA, true)
 */
void FomodViewModel::togglePlugin(const std::shared_ptr<GroupViewModel>&,
    const std::shared_ptr<PluginViewModel>& plugin, const bool selected) const
{
    if (plugin->isSelected() == selected) {
        return;
    }
    plugin->setSelected(selected);
    setFlagForPluginState(plugin, selected);

    if (mInitialized) {
        mActivePlugin = plugin;
        /*
         * Need a way to re-process plugins when flags change
         */
        processPluginConditions();
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
    return mCurrentStepIndex == 0 ? mFomodFile->moduleImage.path : "";
}