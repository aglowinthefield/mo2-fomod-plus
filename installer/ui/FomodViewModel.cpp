#include "FomodViewModel.h"
#include "xml/ModuleConfiguration.h"
#include "lib/Logger.h"

using GroupCallback  = std::function<void(GroupRef)>;
using PluginCallback = std::function<void(GroupRef, PluginRef)>;


/*
--------------------------------------------------------------------------------
                               Helpers
--------------------------------------------------------------------------------
*/
#pragma region Helpers

bool isRadioLike(GroupRef group)
{
    return group->getType() == SelectExactlyOne
        || (group->getType() == SelectAtMostOne && group->getPlugins().size() > 1);
}

bool moreThanOneSelected(GroupRef group)
{
    auto selectedPlugins = group->getPlugins() | std::views::filter([](const auto& plugin) {
        return plugin->isSelected();
    });
    return std::ranges::distance(selectedPlugins) > 1;
}

bool anySelected(GroupRef group)
{
    return std::ranges::any_of(group->getPlugins(), [](const auto& plugin) { return plugin->isSelected(); });
}

std::string pluginTypeEnumToString(const PluginTypeEnum type)
{
    switch (type) {
    case PluginTypeEnum::Recommended:
        return "Recommended";
    case PluginTypeEnum::Required:
        return "Required";
    case PluginTypeEnum::Optional:
        return "Optional";
    case PluginTypeEnum::NotUsable:
        return "NotUsable";
    case PluginTypeEnum::CouldBeUsable:
        return "CouldBeUsable";
    default:
        return "Unknown";
    }
}

#pragma endregion

/*
--------------------------------------------------------------------------------
                               Lifecycle
--------------------------------------------------------------------------------
*/
#pragma region ViewModel Lifecycle

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
    
    // Handle FOMODs with no steps
    if (viewModel->mSteps.empty()) {
        viewModel->mInitialized = true;
        viewModel->logMessage(INFO, "FOMOD with no steps - initialization complete");
        return viewModel;
    }
    
    viewModel->processPluginConditions(-1); // please dont judge me. ill fix this someday.
    viewModel->enforceGroupConstraints();
    viewModel->updateVisibleSteps();
    viewModel->mInitialized      = true;
    viewModel->mCurrentStepIndex = viewModel->mVisibleStepIndices.front();
    viewModel->mActiveStep       = viewModel->mSteps.at(viewModel->mVisibleStepIndices.front());
    viewModel->mActivePlugin     = viewModel->getFirstPluginForActiveStep();
    viewModel->getActiveStep()->setVisited(true);
    viewModel->logMessage(DEBUG, "VIEWMODEL INITIALIZED");
    viewModel->logMessage(DEBUG, viewModel->toString());
    return viewModel;
}
#pragma endregion

/*
--------------------------------------------------------------------------------
                               Traversal Functions
--------------------------------------------------------------------------------
*/
#pragma region Traversal Functions

void FomodViewModel::forEachGroup(const GroupCallback& callback) const
{
    for (const auto& stepViewModel : mSteps) {
        for (const auto& groupViewModel : stepViewModel->getGroups()) {
            callback(groupViewModel);
        }
    }
}

void FomodViewModel::forEachPlugin(const PluginCallback& callback) const
{
    for (const auto& stepViewModel : mSteps) {
        for (const auto& groupViewModel : stepViewModel->getGroups()) {
            for (const auto& pluginViewModel : groupViewModel->getPlugins()) {
                callback(groupViewModel, pluginViewModel);
            }
        }
    }
}

void FomodViewModel::forEachFuturePlugin(const int fromStepIndex, const PluginCallback& callback) const
{
    for (int i = fromStepIndex + 1; i < mSteps.size(); ++i) {
        for (const auto& groupViewModel : mSteps[i]->getGroups()) {
            for (const auto& pluginViewModel : groupViewModel->getPlugins()) {
                callback(groupViewModel, pluginViewModel);
            }
        }
    }
}
#pragma endregion

/*
--------------------------------------------------------------------------------
                               Initialization
--------------------------------------------------------------------------------
*/
#pragma region Initializers
void FomodViewModel::createStepViewModels()
{
    shared_ptr_list<StepViewModel> stepViewModels;

    // Handle legacy FOMODs with no install steps
    if (mFomodFile->installSteps.installSteps.empty()) {
        logMessage(INFO, "No install steps found - creating default step for legacy FOMOD");
        return;
    }

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

                pluginViewModel->setStepIndex(stepIndex);
                pluginViewModel->setGroupIndex(groupIndex);
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

void FomodViewModel::createNonePluginForGroup(GroupRef group)
{
    const auto nonePlugin           = std::make_shared<Plugin>();
    nonePlugin->name                = "None";
    nonePlugin->typeDescriptor.type = PluginTypeEnum::Optional;
    const int newIndex              = static_cast<int>(group->getPlugins().size());
    const auto nonePluginViewModel  = std::make_shared<PluginViewModel>(nonePlugin, true, true, newIndex);
    group->addPlugin(nonePluginViewModel);
}
#pragma endregion

/*
--------------------------------------------------------------------------------
                               Group Constraints
--------------------------------------------------------------------------------
*/
#pragma region Group Constraints

void FomodViewModel::enforceRadioGroupConstraints(GroupRef group) const
{
    if (!isRadioLike(group)) {
        return;
    }

    logMessage(INFO, "Enforcing group constraints for group " + group->getName());

    if (group->getType() == SelectExactlyOne && group->getPlugins().size() == 1) {
        logMessage(INFO,
            "Disabling " + group->getPlugins().at(0)->getName() + " because it's the only plugin.");
        group->getPlugins().at(0)->setEnabled(false);
    }

    if (moreThanOneSelected(group)) {
        logMessage(ERR, "More than one plugin is selected in a SelectExactlyOne group. Deselecting all.");
        for (const auto& plugin : group->getPlugins()) {
            plugin->setSelected(false); // don't call toggle here, that'll do the radio stuff.
        }
    }

    if (anySelected(group)) {
        logMessage(INFO, "At least one plugin is selected. Nothing to enforce.");
        return;
    }

    // First, try to select the first Recommended plugin
    for (const auto& plugin : group->getPlugins()) {
        if (mConditionTester.getPluginTypeDescriptorState(plugin->getPlugin(), mFlags) == PluginTypeEnum::Recommended) {
            logMessage(INFO, "Selecting " + plugin->getName() + " because it's the first recommended plugin.");
            togglePlugin(group, plugin, true);
            return;
        }
    }

    // If no Recommended plugin is found, select the first one that isn't NotUsable
    for (const auto& plugin : group->getPlugins()) {
        if (mConditionTester.getPluginTypeDescriptorState(plugin->getPlugin(), mFlags) != PluginTypeEnum::NotUsable) {
            logMessage(INFO, "Selecting " + plugin->getName() + " because it's the first usable plugin.");
            togglePlugin(group, plugin, true);
            return;
        }
    }
}

void FomodViewModel::enforceSelectAllConstraint(GroupRef groupViewModel) const
{
    if (groupViewModel->getType() != SelectAll) {
        return;
    }

    for (const auto& pluginViewModel : groupViewModel->getPlugins()) {
        togglePlugin(groupViewModel, pluginViewModel, true);
        pluginViewModel->setEnabled(false);
    }

}

void FomodViewModel::enforceSelectAtLeastOneConstraint(GroupRef group) const
{
    if (group->getType() != SelectAtLeastOne || group->getPlugins().size() != 1) {
        return;
    }

    const auto plugin = group->getPlugins().front();
    if (mConditionTester.getPluginTypeDescriptorState(plugin->getPlugin(), mFlags) != PluginTypeEnum::NotUsable) {
        logMessage(DEBUG, "Selecting " + plugin->getName() + " because it's the only plugin in a SelectAtLeastOne.");
        togglePlugin(group, plugin, true);
        plugin->setEnabled(false);
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

#pragma endregion

/*
--------------------------------------------------------------------------------
                               Plugin Constraints
--------------------------------------------------------------------------------
*/
#pragma region Plugin Constraints

void FomodViewModel::processPlugin(GroupRef group, PluginRef plugin) const
{
    if (group->getType() == SelectAll) {
        return;
    }
    const auto typeDescriptor = mConditionTester.getPluginTypeDescriptorState(plugin->plugin, mFlags);

    if (typeDescriptor == plugin->getCurrentPluginType()) {
        return;
    }

    logMessage(DEBUG,
        "Plugin " + plugin->getName() + " in group " + std::to_string(group->getOwnIndex()) + " has changed type from "
        + pluginTypeEnumToString(plugin->getCurrentPluginType()) + " to " + pluginTypeEnumToString(typeDescriptor));

    plugin->setCurrentPluginType(typeDescriptor);

    const bool isOnlyPlugin = group->getPlugins().size() == 1
        && (group->getType() == SelectExactlyOne || group->getType() == SelectAtLeastOne);

    // check if step hasVisited, if it hasn't been, set it to unchecked if it's optional.
    const auto stepNotVisitedYet = !mSteps[group->getStepIndex()]->getHasVisited();

    switch (typeDescriptor) {
    case PluginTypeEnum::Recommended:
        plugin->setEnabled(true);
        if (!plugin->isSelected()) {
            togglePlugin(group, plugin, true);
        }
        break;
    case PluginTypeEnum::Required:
        plugin->setEnabled(false);
        if (!plugin->isSelected()) {
            togglePlugin(group, plugin, true);
        }
        break;
    case PluginTypeEnum::Optional:
        if (!isOnlyPlugin) {
            plugin->setEnabled(true);
        }
    // In the case where we're changing flags to make something optional from Recommended, set it back to unchecked.
        if (plugin->isSelected() & stepNotVisitedYet && group->getType() == SelectAny) {
            togglePlugin(group, plugin, false);
        }
        break;
    case PluginTypeEnum::NotUsable:
        plugin->setEnabled(false);
        if (plugin->isSelected()) {
            togglePlugin(group, plugin, false);
        }
        break;
    case PluginTypeEnum::CouldBeUsable:
        plugin->setEnabled(true);
        break;
    default: ;
    }
}

void FomodViewModel::processPluginConditions(const int fromStepIndex) const
{
    // We only want to update plugins that haven't been seen yet. Otherwise, we could undo manual selections by the user.
    if (fromStepIndex >= 0) {
        logMessage(DEBUG, "Processing plugins from step " + std::to_string(fromStepIndex));
        forEachFuturePlugin(fromStepIndex, [this](const auto& groupViewModel, const auto& pluginViewModel) {
            processPlugin(groupViewModel, pluginViewModel);
        });
    } else {
        forEachPlugin([this](const auto& groupViewModel, const auto& pluginViewModel) {
            processPlugin(groupViewModel, pluginViewModel);
        });
    }
}

void FomodViewModel::setFlagForPluginState(PluginRef plugin) const
{
    if (plugin->isSelected()) {
        mFlags->setFlagsForPlugin(plugin);
    } else {
        mFlags->unsetFlagsForPlugin(plugin);
    }
}

/*
 * In an exclusive group, this gets called for the deselected plugin and then the selected plugin.
 * So if we're unselecting modB to select modA, we will get calls like
 *  togglePlugin(group, modB, false)
 *  togglePlugin(group, modA, true)
 */
bool FomodViewModel::togglePlugin(GroupRef group, PluginRef plugin, const bool selected) const
{
    if (plugin->isSelected() == selected) {
        logMessage(DEBUG, "Plugin " + plugin->getName() + " is already " + (selected ? "selected" : "deselected"));
        return false;
    }

    // Disable other radio options first.
    if (selected && isRadioLike(group)) {
        for (const auto& otherPlugin : group->getPlugins()) {
            if (otherPlugin != plugin && plugin->isSelected()) {
                logMessage(DEBUG,
                    "Deselecting " + otherPlugin->getName() + " because " + plugin->getName() + " was selected.");
                otherPlugin->setSelected(false);
                setFlagForPluginState(otherPlugin);
            }
        }
    }

    const auto stepIndex = group->getStepIndex();

    logMessage(INFO, "Toggling " + plugin->getName() + " to " + (selected ? "true" : "false"));
    plugin->setSelected(selected);
    setFlagForPluginState(plugin);

    if (mInitialized) {
        mActivePlugin = plugin;
    }
    processPluginConditions(stepIndex);
    updateVisibleSteps();
    return true;
}

void FomodViewModel::markManuallySet(PluginRef plugin)
{
    plugin->manuallySet = true;
}

#pragma endregion

/*
--------------------------------------------------------------------------------
                               Step Constraints
--------------------------------------------------------------------------------
*/
#pragma region Step Constraints

void FomodViewModel::updateVisibleSteps() const
{
    mVisibleStepIndices.clear();
    mFlags->clearAll();

    for (int i = 0; i < mSteps.size(); ++i) {
        if (i == 0) {
            rebuildConditionFlagsForStep(i);
        }

        // This also depends on previous flags that may have set this particular flag.
        if (mConditionTester.isStepVisible(mFlags, mSteps[i]->getVisibilityConditions(), i, mSteps)) {
            mVisibleStepIndices.push_back(i);
            rebuildConditionFlagsForStep(i);
        }
    }
    if (mFlags->getFlagCount() > 0) {
        logMessage(DEBUG, mFlags->toString());
    }
}

void FomodViewModel::rebuildConditionFlagsForStep(const int stepIndex) const
{
    for (const auto& group : mSteps[stepIndex]->getGroups()) {
        for (const auto& plugin : group->getPlugins()) {
            setFlagForPluginState(plugin);
        }
    }
}
#pragma endregion

/*
--------------------------------------------------------------------------------
                               Navigation/UI
--------------------------------------------------------------------------------
*/
#pragma region Navigation/UI

void FomodViewModel::stepBack()
{
    if (mSteps.empty()) {
        return;  // No steps to move back to
    }
    
    logMessage(DEBUG, "Stepping back from step " + std::to_string(mCurrentStepIndex));
    const auto it = std::ranges::find(mVisibleStepIndices, mCurrentStepIndex);
    if (it != mVisibleStepIndices.end() && it != mVisibleStepIndices.begin()) {
        mCurrentStepIndex = *std::prev(it);
        mActiveStep       = mSteps[mCurrentStepIndex];
        mActivePlugin     = getFirstPluginForActiveStep();
    }
    logMessage(DEBUG, "Stepped back to step " + std::to_string(mCurrentStepIndex));
}

void FomodViewModel::stepForward()
{
    if (mSteps.empty()) {
        return;  // No steps to move forward to
    }
    
    logMessage(DEBUG, "Stepping forward from step " + std::to_string(mCurrentStepIndex));
    const auto it = std::ranges::find(mVisibleStepIndices, mCurrentStepIndex);
    if (it != mVisibleStepIndices.end() && std::next(it) != mVisibleStepIndices.end()) {
        mCurrentStepIndex = *std::next(it);
        mActiveStep       = mSteps[mCurrentStepIndex];
        mActivePlugin     = getFirstPluginForActiveStep();
    }
    mActiveStep->setVisited(true);
    logMessage(DEBUG, "Stepped forward to step " + std::to_string(mCurrentStepIndex));
}

bool FomodViewModel::isLastVisibleStep() const
{
    if (mSteps.empty()) {
        return true;  // Legacy FOMODs are always "last step"
    }
    return !mVisibleStepIndices.empty() && mCurrentStepIndex == mVisibleStepIndices.back();
}

bool FomodViewModel::isFirstVisibleStep() const
{
    if (mSteps.empty()) {
        return true;  // Legacy FOMODs are always "first step"
    }
    return !mVisibleStepIndices.empty() && mCurrentStepIndex == mVisibleStepIndices.front();
}

void FomodViewModel::preinstall(const std::shared_ptr<MOBase::IFileTree>& tree, const QString& fomodPath)
{
    mFileInstaller = std::make_shared<
        FileInstaller>(mOrganizer, fomodPath, tree, std::move(mFomodFile), mFlags, mSteps);
}


std::string FomodViewModel::getDisplayImage() const
{
    // if the active plugin has an image, return it
    if (mActivePlugin && !mActivePlugin->getImagePath().empty()) {
        return mActivePlugin->getImagePath();
    }
    return mCurrentStepIndex == 0 ? mFomodFile->moduleImage.path : "";
}

std::shared_ptr<PluginViewModel> FomodViewModel::getFirstPluginForActiveStep() const
{
    if (!mActiveStep) {
        return nullptr;
    }

    const auto& groups = mActiveStep->getGroups();
    if (groups.empty()) {
        return nullptr;
    }

    const auto& plugins = groups.front()->getPlugins();
    if (plugins.empty()) {
        return nullptr;
    }

    return plugins.front();
}
#pragma endregion

/*
--------------------------------------------------------------------------------
                               Utility
--------------------------------------------------------------------------------
*/
#pragma region Utility
std::string FomodViewModel::toString() const
{
    std::string viewModel = "\n";
    for (const auto& step : mSteps) {

        const auto isVisible = std::ranges::find(mVisibleStepIndices, step->getOwnIndex()) != mVisibleStepIndices.end();
        viewModel += "Step " + std::to_string(step->getOwnIndex()) + ": " + step->getName() + "[Visible: " +
            std::to_string(isVisible) + "]\n";

        for (const auto& group : step->getGroups()) {

            viewModel += "\tGroup " + std::to_string(group->getOwnIndex()) + ": " + group->getName() + "\n";

            for (const auto& plugin : group->getPlugins()) {
                viewModel += "\t\tPlugin: " + plugin->getName() + "[Selected: " + (plugin->isSelected()
                    ? "TRUE"
                    : "FALSE") + "]\n";
            }
        }
    }
    viewModel += "\n";
    std::ostringstream oss;
    std::ranges::transform(mVisibleStepIndices,
        std::ostream_iterator<std::string>(oss, ", "),
        [](const int i) { return std::to_string(i); });

    std::string stepList = oss.str();
    stepList.erase(stepList.length() - 2);

    viewModel += "Visible Steps: [" + stepList + "]\n";
    viewModel += mFlags->toString();
    return viewModel;
}


void FomodViewModel::resetToDefaults()
{
    logMessage(INFO, "Resetting all choices to author defaults");

    // Clear all flags first
    mFlags->clearAll();

    // Reset all plugins to deselected and clear visited states
    for (const auto& step : mSteps) {
        step->setVisited(false);
        for (const auto& group : step->getGroups()) {
            for (const auto& plugin : group->getPlugins()) {
                plugin->setSelected(false);
                plugin->setEnabled(true);
                plugin->manuallySet = false;
                plugin->setCurrentPluginType(PluginTypeEnum::UNKNOWN);
            }
        }
    }

    // Re-run the initial constraint enforcement to restore author defaults
    processPluginConditions(-1);
    enforceGroupConstraints();
    updateVisibleSteps();

    // Reset to first step
    mCurrentStepIndex = mVisibleStepIndices.empty() ? 0 : mVisibleStepIndices.front();
    mActiveStep       = mSteps.empty() ? nullptr : mSteps.at(mCurrentStepIndex);
    mActivePlugin     = getFirstPluginForActiveStep();
    if (mActiveStep) {
        mActiveStep->setVisited(true);
    }

    logMessage(DEBUG, "Reset complete. Current state:\n" + toString());
}

void FomodViewModel::selectFromJson(nlohmann::json json) const
{
    const auto jsonSteps = json["steps"];
    const auto stepCount = jsonSteps.size();

    for (int stepIndex = 0; stepIndex < stepCount; ++stepIndex) {

        if (stepIndex > mSteps.size() - 1) {
            logMessage(ERR, "Step index " + std::to_string(stepIndex) + " is out of bounds.");
            continue;
        }

        const auto currentStep = mSteps[stepIndex];
        const auto step        = jsonSteps[stepIndex];
        const auto groupCount  = step["groups"].size();

        logMessage(DEBUG, "Selecting plugins for step " + std::to_string(stepIndex));
        logMessage(DEBUG, "There are " + std::to_string(groupCount) + " groups.");

        for (int groupIndex = 0; groupIndex < groupCount; ++groupIndex) {
            if (groupIndex > currentStep->getGroups().size() - 1) {
                logMessage(ERR, "Group index " + std::to_string(groupIndex) + " is out of bounds.");
                continue;
            }

            const auto group        = step["groups"][groupIndex];
            const auto currentGroup = currentStep->getGroups()[groupIndex];

            for (const auto jsonPlugin : group["plugins"]) {

                const auto& allPlugins = currentGroup->getPlugins();
                const auto searchName = jsonPlugin.get<std::string>();

                logMessage(DEBUG, "Looking for plugin " + searchName);

                const auto currentPlugin = std::ranges::find_if(allPlugins,
                    [searchName](PluginRef p) {
                        return p->getName() == searchName;
                    });

                if (currentPlugin == allPlugins.end()) {
                    logMessage(DEBUG, "Plugin " + searchName + " not found in group " + currentGroup->getName());
                    continue;
                }

                if ((*currentPlugin)->isSelected()) {
                    logMessage(DEBUG, "Plugin " + searchName + " is already selected.");
                    continue;
                }
                logMessage(DEBUG, "Toggle plugin " + searchName + " to selected.");
                if (!(*currentPlugin)->isEnabled()) {
                    logMessage(DEBUG, "Plugin " + searchName + " is not enabled.");
                    continue;
                }
                togglePlugin(currentGroup, *currentPlugin, true);
            }

            if (!group.contains("deselected")) {
                continue;
            }

            // Do the opposite of the above. For unchecked plugins, disable them.
            for (const auto jsonPlugin : group["deselected"]) {

                const auto& allPlugins = currentGroup->getPlugins();
                const auto searchName = jsonPlugin.get<std::string>();

                logMessage(DEBUG, "Looking for plugin to disable: " + searchName);

                const auto currentPlugin = std::ranges::find_if(allPlugins,
                    [searchName](PluginRef p) {
                        return p->getName() == searchName;
                    });

                if (currentPlugin == allPlugins.end()) {
                    logMessage(DEBUG, "Plugin " + searchName + " not found in group " + currentGroup->getName());
                    continue;
                }

                if (!(*currentPlugin)->isSelected()) {
                    logMessage(DEBUG, "Plugin " + searchName + " is already deselected.");
                    continue;
                }
                logMessage(DEBUG, "Toggle plugin " + searchName + " to deselected.");
                if (!(*currentPlugin)->isEnabled()) {
                    logMessage(DEBUG, "Plugin " + searchName + " is not enabled.");
                    continue;
                }
                togglePlugin(currentGroup, *currentPlugin, false);
                (*currentPlugin)->manuallySet = true; // To preserve this state when serializing JSON.
            }
        }
    }
}
#pragma endregion
