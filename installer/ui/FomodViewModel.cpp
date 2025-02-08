#include "FomodViewModel.h"
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
    viewModel->processPluginConditions(0);
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
    const int fromStepIndex,
    const std::function<void(const std::shared_ptr<GroupViewModel>&, const std::shared_ptr<PluginViewModel>&)>&
    callback) const
{
    for (int i = 0; i < mSteps.size(); ++i) {
        if (i <= fromStepIndex) {
            continue;
        }
        for (const auto& groupViewModel : mSteps[i]->getGroups()) {
            for (const auto& pluginViewModel : groupViewModel->getPlugins()) {
                callback(groupViewModel, pluginViewModel);
            }
        }
    }
}

void FomodViewModel::selectFromJson(nlohmann::json json) const
{

    const auto jsonSteps = json["steps"];

    for (int stepIndex = 0; stepIndex < jsonSteps.size(); ++stepIndex) {

        const auto currentStep = mSteps[stepIndex];
        const auto& step = jsonSteps[stepIndex];

        for (int groupIndex = 0; groupIndex < step["groups"].size(); ++groupIndex) {

            const auto& group        = step["groups"][groupIndex];
            const auto currentGroup = currentStep->getGroups()[groupIndex];

            for (const auto & jsonPlugin : group["plugins"]) {

                const auto allPlugins = currentGroup->getPlugins();
                log.logMessage(DEBUG, "Looking for plugin " + jsonPlugin.get<std::string>());

                const auto currentPlugin = std::ranges::find_if(allPlugins,
                    [&jsonPlugin](const std::shared_ptr<PluginViewModel>& p) {
                        return p->getName() == jsonPlugin.get<std::string>();
                    });

                if (currentPlugin == allPlugins.end()) {
                    continue;
                }

                if ((*currentPlugin)->isSelected()) {
                    continue;
                }
                togglePlugin(currentGroup, *currentPlugin, true);
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

    log.logMessage(INFO, "Enforcing group constraints for group " + groupViewModel->getName());

    if (groupViewModel->getType() == SelectExactlyOne && groupViewModel->getPlugins().size() == 1) {
        log.logMessage(INFO,
            "Disabling " + groupViewModel->getPlugins().at(0)->getName() + " because it's the only plugin.");
        groupViewModel->getPlugins().at(0)->setEnabled(false);
    }

    if (std::ranges::any_of(groupViewModel->getPlugins(), [](const auto& plugin) { return plugin->isSelected(); })) {
        log.logMessage(INFO, "At least one plugin is selected. Nothing to enforce.");
        return; // We're good if at least one is selected.
    }

    // First, try to select the first Recommended plugin
    for (const auto& plugin : groupViewModel->getPlugins()) {
        if (mConditionTester.getPluginTypeDescriptorState(plugin->getPlugin(), mFlags) == PluginTypeEnum::Recommended) {
            log.logMessage(INFO, "Selecting " + plugin->getName() + " because it's the first recommended plugin.");
            togglePlugin(groupViewModel, plugin, true);
            return;
        }
    }

    // If no Recommended plugin is found, select the first one that isn't NotUsable
    for (const auto& plugin : groupViewModel->getPlugins()) {
        if (mConditionTester.getPluginTypeDescriptorState(plugin->getPlugin(), mFlags) != PluginTypeEnum::NotUsable) {
            log.logMessage(INFO, "Selecting " + plugin->getName() + " because it's the first usable plugin.");
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
        const auto plugin = groupViewModel->getPlugins().at(0);
        if (mConditionTester.getPluginTypeDescriptorState(plugin->getPlugin(), mFlags) != PluginTypeEnum::NotUsable) {
            log.logMessage(DEBUG, "Selecting " + plugin->getName() + " because it's the only plugin in a SelectAtLeastOne.");
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

    const bool isOnlyPlugin = groupViewModel->getPlugins().size() == 1
        && (groupViewModel->getType() == SelectExactlyOne || groupViewModel->getType() == SelectAtLeastOne);

    switch (typeDescriptor) {
    case PluginTypeEnum::Recommended:
        pluginViewModel->setEnabled(true);
        if (!pluginViewModel->isSelected()) {
            togglePlugin(groupViewModel, pluginViewModel, true);
        }
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

void FomodViewModel::processPluginConditions(const int fromStepIndex) const
{
    // We only want to update plugins that haven't been seen yet. Otherwise we could undo manual selections by the user.
    if (mInitialized) {
        forEachFuturePlugin(fromStepIndex, [this](const auto& groupViewModel, const auto& pluginViewModel) {
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
        const auto flagValue = selected ? flag.value : "";
        log.logMessage(DEBUG, "Setting flag " + flag.name + " to " + flagValue);
        mFlags->setFlag(flag.name, flagValue);
        log.logMessage(DEBUG, "Flag " + flag.name + " set? " + mFlags->getFlag(flag.name));
    }
}

/*
 * In an exclusive group, this gets called for the deselected plugin and then the selected plugin.
 * So if we're unselecting modB to select modA, we will get calls like
 *  togglePlugin(group, modB, false)
 *  togglePlugin(group, modA, true)
 */
void FomodViewModel::togglePlugin(const std::shared_ptr<GroupViewModel>& group,
    const std::shared_ptr<PluginViewModel>& plugin, const bool selected) const
{
    if (plugin->isSelected() == selected) {
        log.logMessage(DEBUG, "Plugin " + plugin->getName() + " is already " + (selected ? "selected" : "deselected"));
        return;
    }

    // Disable other radio options first.
    bool isRadioLike = group->getType() == SelectExactlyOne || (group->getType() == SelectAtMostOne && group->
        getPlugins().size() > 1);

    if (selected && isRadioLike) {
        for (const auto& pluginViewModel : group->getPlugins()) {
            if (pluginViewModel != plugin) {
                log.logMessage(DEBUG,
                    "Deselecting " + pluginViewModel->getName() + " because " + plugin->getName() + " was selected.");
                pluginViewModel->setSelected(false);
                setFlagForPluginState(pluginViewModel, false);
            }
        }
    }

    const auto stepIndex = group->getStepIndex();

    log.logMessage(INFO, "[VIEWMODEL] Toggling " + plugin->getName() + " to " + (selected ? "true" : "false"));
    plugin->setSelected(selected);
    setFlagForPluginState(plugin, selected);


    if (mInitialized) {
        mActivePlugin = plugin;
        processPluginConditions(stepIndex);
        updateVisibleSteps();
    }
}

void FomodViewModel::updateVisibleSteps() const
{
    mVisibleStepIndices.clear();
    for (int i = 0; i < mSteps.size(); ++i) {

        // This also depends on previous flags that may have set this particular flag.
        if (mConditionTester.isStepVisible(mFlags, mSteps[i]->getVisibilityConditions(), i, mSteps)) {
            mVisibleStepIndices.push_back(i);
        } else {
            log.logMessage(DEBUG, "Step " + std::to_string(i) + " is NOT visible.");
            mFlags->forEach([this](const std::string& flag, const std::string& value) {
                log.logMessage(DEBUG, "Flag: " + flag + ", Value: " + value);
            });
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