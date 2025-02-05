﻿#include "FomodViewModel.h"
#include "xml/ModuleConfiguration.h"
#include "lib/Logger.h"

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
    viewModel->enforceGroupConstraints();
    viewModel->processPluginConditions();
    viewModel->updateVisibleSteps();
    viewModel->mInitialized  = true;
    viewModel->mActiveStep   = viewModel->mSteps.at(0);
    viewModel->mActivePlugin = viewModel->getFirstPluginForActiveStep();
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

void FomodViewModel::createStepViewModels()
{
    shared_ptr_list<StepViewModel> stepViewModels;

    for (int stepIndex = 0; stepIndex < mFomodFile->installSteps.installSteps.size(); ++stepIndex) {
        const auto& installStep = mFomodFile->installSteps.installSteps[stepIndex];
        shared_ptr_list<GroupViewModel> groupViewModels;

        for (int groupIndex = 0; groupIndex < installStep.optionalFileGroups.groups.size(); ++groupIndex) {
            const auto& group = installStep.optionalFileGroups.groups[groupIndex];
            shared_ptr_list<PluginViewModel> pluginViewModels;

            for (int pluginIndex = 0; pluginIndex < group.plugins.plugins.size(); ++pluginIndex) {
                const auto& plugin   = group.plugins.plugins[pluginIndex];
                auto pluginViewModel = std::make_shared<PluginViewModel>(std::make_shared<Plugin>(plugin), false, true,
                    pluginIndex);
                pluginViewModels.emplace_back(pluginViewModel); // Assuming default values for selected and enabled
            }
            auto groupViewModel = std::make_shared<GroupViewModel>(std::make_shared<Group>(group), pluginViewModels,
                groupIndex, stepIndex);
            if (groupViewModel->getType() == SelectAtMostOne && groupViewModel->getPlugins().size() > 1) {
                createNonePluginForGroup(groupViewModel);
            }
            groupViewModels.emplace_back(groupViewModel);
        }
        auto stepViewModel = std::make_shared<StepViewModel>(std::make_shared<InstallStep>(installStep),
            std::move(groupViewModels), stepIndex);
        stepViewModels.emplace_back(stepViewModel);

    }
    mSteps = std::move(stepViewModels);
}

/*
--------------------------------------------------------------------------------
                               Group Constraints
--------------------------------------------------------------------------------
*/
// ReSharper disable once CppMemberFunctionMayBeStatic
void FomodViewModel::createNonePluginForGroup(const std::shared_ptr<GroupViewModel>& group) const
{
    const auto nonePlugin           = std::make_shared<Plugin>();
    nonePlugin->name                = "None";
    nonePlugin->typeDescriptor.type = PluginTypeEnum::Optional;
    const int newIndex              = static_cast<int>(group->getPlugins().size());
    const auto nonePluginViewModel  = std::make_shared<PluginViewModel>(nonePlugin, true, true, newIndex);
    group->addPlugin(nonePluginViewModel);
}

void FomodViewModel::enforceRadioGroupConstraints(const std::shared_ptr<GroupViewModel>& groupViewModel) const
{
    if (groupViewModel->getType() != SelectExactlyOne) {
        return;
    }

    std::cout << "Enforcing group constraints for group " << groupViewModel->getName() << std::endl;

    if (groupViewModel->getType() == SelectExactlyOne && groupViewModel->getPlugins().size() == 1) {
        std::cout << "Disabling " << groupViewModel->getPlugins().at(0)->getName() << " because it's the only plugin."
            << std::endl;
        groupViewModel->getPlugins().at(0)->setEnabled(false);
    }

    if (std::ranges::any_of(groupViewModel->getPlugins(), [](const auto& plugin) { return plugin->isSelected(); })) {
        std::cout << "At least one plugin is selected. Nothing to enforce." << std::endl;
        return; // We're good if at least one is selected.
    }

    // First, try to select the first Recommended plugin
    for (const auto& plugin : groupViewModel->getPlugins()) {
        if (mConditionTester.getPluginTypeDescriptorState(plugin->getPlugin(), mFlags) == PluginTypeEnum::Recommended) {
            std::cout << "Selecting " << plugin->getName() << " because it's the first recommended plugin." <<
                std::endl;
            togglePlugin(groupViewModel, plugin, true);
            return;
        }
    }

    // If no Recommended plugin is found, select the first one that isn't NotUsable
    for (const auto& plugin : groupViewModel->getPlugins()) {
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

/*
--------------------------------------------------------------------------------
                               Plugin Constraints
--------------------------------------------------------------------------------
*/
// TODO: This should be a group-based thing
void FomodViewModel::processPlugin(const std::shared_ptr<GroupViewModel>& groupViewModel,
    const std::shared_ptr<PluginViewModel>& pluginViewModel) const
{
    // Might be a workaround. lets see.
    if (groupViewModel->getType() == SelectAll) {
        return;
    }
    const auto typeDescriptor = mConditionTester.getPluginTypeDescriptorState(pluginViewModel->plugin, mFlags);
    // std::cout << "Processing plugin " << pluginViewModel->getName() << " with type " << typeDescriptor << std::endl;

    const bool isOnlyPlugin = groupViewModel->getPlugins().size() == 1
        && (groupViewModel->getType() == SelectExactlyOne || groupViewModel->getType() == SelectAtLeastOne);

    switch (typeDescriptor) {
    case PluginTypeEnum::Recommended:
        pluginViewModel->setEnabled(true);
        togglePlugin(groupViewModel, pluginViewModel, true);
        break;
    case PluginTypeEnum::Required:
        pluginViewModel->setEnabled(false);
        if (!pluginViewModel->isSelected()) {
            togglePlugin(groupViewModel, pluginViewModel, true);
        }
        break;
    case PluginTypeEnum::Optional:
        if (!isOnlyPlugin) {
            pluginViewModel->setEnabled(true);
        }
        break;
    case PluginTypeEnum::NotUsable:
        pluginViewModel->setEnabled(false);
        if (pluginViewModel->isSelected()) {
            togglePlugin(groupViewModel, pluginViewModel, false);
        }
        break;
    case PluginTypeEnum::CouldBeUsable:
        pluginViewModel->setEnabled(true);
        break;
    default: ;
    }
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
        processPluginConditions();
        updateVisibleSteps();
    }
}

void FomodViewModel::updateVisibleSteps() const
{
    mVisibleStepIndices.clear();
    for (int i = 0; i < mSteps.size(); ++i) {
        if (mConditionTester.testCompositeDependency(mFlags, mSteps[i]->getVisibilityConditions())) {
            mVisibleStepIndices.push_back(i);
        }
    }
}

/*
--------------------------------------------------------------------------------
                               Navigation/UI
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

bool FomodViewModel::isLastVisibleStep() const
{
    return !mVisibleStepIndices.empty() && mCurrentStepIndex == mVisibleStepIndices.back();
}

void FomodViewModel::preinstall(const std::shared_ptr<MOBase::IFileTree>& tree, const QString& fomodPath)
{
    mFileInstaller = std::make_shared<
        FileInstaller>(mOrganizer, fomodPath, tree, std::move(mFomodFile), mFlags, mSteps);
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

const std::shared_ptr<PluginViewModel>& FomodViewModel::getFirstPluginForActiveStep() const
{
    return mActiveStep->getGroups().at(0)->getPlugins().at(0);
}