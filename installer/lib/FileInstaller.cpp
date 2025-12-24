#include "FileInstaller.h"

#include <utility>

#include "ui/FomodViewModel.h"

using namespace MOBase;

FileInstaller::FileInstaller(
    IOrganizer* organizer,
    QString fomodPath,
    const std::shared_ptr<IFileTree>& fileTree,
    std::unique_ptr<ModuleConfiguration> fomodFile,
    const std::shared_ptr<FlagMap>& flagMap,
    const std::vector<std::shared_ptr<StepViewModel> >& steps) : mOrganizer(organizer),
                                                                 mFomodPath(std::move(fomodPath)),
                                                                 mFileTree(fileTree),
                                                                 mFomodFile(std::move(fomodFile)),
                                                                 mFlagMap(flagMap),
                                                                 mConditionTester(organizer), mSteps(steps)
{
    const auto requiredCount =
        (mFomodFile != nullptr) ? mFomodFile->requiredInstallFiles.files.size() : 0;
    const auto stepCount = mSteps.size();
    logMessage(DEBUG,
               "FileInstaller constructed. steps=" + std::to_string(stepCount) + ", required files=" +
                   std::to_string(requiredCount) + ", fomodPath=" + mFomodPath.toStdString());
}

std::shared_ptr<IFileTree> FileInstaller::install() const
{
    logMessage(DEBUG, "Starting FileInstaller::install()");
    const auto filesToInstall = collectFilesToInstall();
    logMessage(INFO, "Installing " + std::to_string(filesToInstall.size()) + " files");
    logMessage(INFO, "FlagMap");
    logMessage(INFO, mFlagMap->toString());

    // update the file tree with the new files
    const std::shared_ptr<IFileTree> installTree = mFileTree->createOrphanTree();

    for (const auto& file : filesToInstall) {

        logMessage(DEBUG,
                   "Processing install entry source=" + file.source + ", destination=" +
                       (file.destination.has_value() ? file.destination.value() : "<default>") +
                       ", priority=" + std::to_string(file.priority));
        const auto sourcePath = getQualifiedFilePath(file.source);
        const auto sourceNode = mFileTree->find(QString::fromStdString(sourcePath));
        if (sourceNode == nullptr) {
            logMessage(ERR, "Could not find source: " + file.source);
            continue;
        }
        const auto targetPath = file.destination.has_value()
            ? QString::fromStdString(file.destination.value())
            : QString::fromStdString(sourcePath);

        // If it's a folder, copy the contents of the folder, not the folder itself.
        if (sourceNode->isDir()) {
            logMessage(DEBUG,
                       "Copying directory '" + file.source + "' into target '" + targetPath.toStdString() +
                           "'");
            // TODO: Check if target path is literally undefined/null
            const auto& tree = sourceNode->astree();
            for (auto it = tree->begin(); it != tree->end(); ++it) {
                const auto& entry = *it;
                const auto path   = targetPath.isEmpty() ? entry->name() : targetPath + "/" + entry->name();
                logMessage(DEBUG,
                           "  Copying entry '" + entry->name().toStdString() + "' to '" +
                               path.toStdString() + "'");
                installTree->copy(entry, path, IFileTree::InsertPolicy::MERGE);
            }
        } else {
            logMessage(DEBUG,
                       "Copying file '" + file.source + "' to '" + targetPath.toStdString() + "'");
            installTree->copy(sourceNode, targetPath, IFileTree::InsertPolicy::MERGE);
        }
    }

    // This file will be written by the InstallationManager later.
    const auto jsonFilePath = "fomod.json";
    installTree->addFile(QString::fromStdString(jsonFilePath), true);
    logMessage(DEBUG, "Added fomod.json placeholder file to install tree.");

    logMessage(DEBUG, "FileInstaller::install completed.");
    return installTree;
}

nlohmann::json FileInstaller::generateFomodJson() const
{
    nlohmann::json fomodJson;
    logMessage(DEBUG, "Generating fomod.json representation for " + std::to_string(mSteps.size()) +
                          " steps.");

    fomodJson["steps"] = nlohmann::json::array();
    for (const auto& stepViewModel : mSteps) {
        auto stepJson      = nlohmann::json::object();
        stepJson["name"]   = stepViewModel->getName();
        stepJson["groups"] = nlohmann::json::array();
        logMessage(DEBUG, "Serializing step '" + stepViewModel->getName() + "'");

        for (const auto& groupViewModel : stepViewModel->getGroups()) {
            auto groupJson       = nlohmann::json::object();
            groupJson["name"]    = groupViewModel->getName();
            auto pluginArray     = nlohmann::json::array();
            auto deselectedArray = nlohmann::json::array();
            logMessage(DEBUG,
                       "  Serializing group '" + groupViewModel->getName() +
                           "' with " + std::to_string(groupViewModel->getPlugins().size()) + " plugins");

            for (const auto& pluginViewModel : groupViewModel->getPlugins()) {
                logMessage(DEBUG,
                           "    Plugin '" + pluginViewModel->getName() + "' selected=" +
                               (pluginViewModel->isSelected() ? "true" : "false") + ", manually-set=" +
                               (pluginViewModel->wasManuallySet() ? "true" : "false"));
                if (pluginViewModel->isSelected()) {
                    pluginArray.emplace_back(pluginViewModel->getName());
                }
                // Add deselected plugins here. (TODO: This will be replaced with an embedded db.)
                if (!pluginViewModel->isSelected() && pluginViewModel->wasManuallySet()) {
                    deselectedArray.emplace_back(pluginViewModel->getName());
                }
            }
            groupJson["plugins"]    = pluginArray;
            groupJson["deselected"] = deselectedArray;
            stepJson["groups"].emplace_back(groupJson);
        }
        fomodJson["steps"].emplace_back(stepJson);
    }
    return fomodJson;
}

std::string FileInstaller::getQualifiedFilePath(const std::string& treePath) const
{
    // We need to prepend the fomod path to whatever source we reference. Guess we're passing that path around.
    const auto qualifiedPath = mFomodPath.toStdString() + "/" + treePath;
    logMessage(DEBUG, "Qualifying tree path '" + treePath + "' to '" + qualifiedPath + "'");
    return qualifiedPath;
}

std::vector<std::string> FileInstaller::collectPositiveFileNamesFromDependencyPatterns(
    const std::vector<DependencyPattern>& patterns)
{
    std::vector<std::string> usableFileDependencyPluginNames = {};
    logMessage(DEBUG,
               "Collecting positive file names from " + std::to_string(patterns.size()) + " patterns.");

    for (const auto& pattern : patterns) {
        if (pattern.type == PluginTypeEnum::NotUsable) {
            logMessage(DEBUG, "Skipping pattern marked as NotUsable.");
            continue;
        }

        if (pattern.dependencies.fileDependencies.empty() && pattern.dependencies.nestedDependencies.empty()) {
            logMessage(DEBUG, "Skipping pattern with no dependencies.");
            continue;
        }

        const auto fileDependencies   = pattern.dependencies.fileDependencies;
        const auto nestedDependencies = pattern.dependencies.nestedDependencies;

        for (const auto& fileDependency : fileDependencies) {
            if (fileDependency.state != FileDependencyTypeEnum::Active) {
                continue;
            }
            usableFileDependencyPluginNames.emplace_back(fileDependency.file);
            logMessage(DEBUG, "Adding active file dependency: " + fileDependency.file);
        }

        for (const auto& nestedDependency : nestedDependencies) {
            for (const auto& fileDependency : nestedDependency.fileDependencies) {
                if (fileDependency.state != FileDependencyTypeEnum::Active) {
                    continue;
                }
                usableFileDependencyPluginNames.emplace_back(fileDependency.file);
                logMessage(DEBUG, "Adding nested active file dependency: " + fileDependency.file);
            }
        }
        // Not handling twice-nested dependencies now. IDK if that's even feasible.
    }

    logMessage(DEBUG,
               "CollectPositiveFileNamesFromDependencyPatterns found " +
                   std::to_string(usableFileDependencyPluginNames.size()) + " entries.");
    return usableFileDependencyPluginNames;
}

// Generic vector appender
void FileInstaller::addFiles(std::vector<File>& main, std::vector<File> toAdd) const
{
    for (const auto& add : toAdd) {
        logMessage(INFO, "Adding file with source: " + add.source);
    }
    main.insert(main.end(), toAdd.begin(), toAdd.end());
    logMessage(DEBUG,
               "addFiles merged " + std::to_string(toAdd.size()) + " files. Total is now " +
                   std::to_string(main.size()));
}

// TODO: Unclear if we're copying. oh well.
// TODO: Rebuild flagmap and step indeces before installing
std::vector<File> FileInstaller::collectFilesToInstall() const
{
    logMessage(DEBUG, "collectFilesToInstall started.");
    std::vector<File> allFiles;

    // Required files from FOMOD
    const FileList requiredInstallFiles = mFomodFile->requiredInstallFiles;
    logMessage(DEBUG,
               "Adding " + std::to_string(requiredInstallFiles.files.size()) +
                   " required install files from fomod.");
    addFiles(allFiles, requiredInstallFiles.files);

    // Selected files from visible steps
    for (const auto& stepViewModel : mSteps) {
        if (!mConditionTester.testCompositeDependency(mFlagMap, stepViewModel->getVisibilityConditions())) {
            logMessage(DEBUG,
                       "Skipping invisible step '" + stepViewModel->getName() + "'");
            continue;
        }
        logMessage(DEBUG,
                   "Processing visible step '" + stepViewModel->getName() + "' with " +
                       std::to_string(stepViewModel->getGroups().size()) + " groups.");
        for (const auto& groupViewModel : stepViewModel->getGroups()) {
            logMessage(DEBUG,
                       " Processing group '" + groupViewModel->getName() + "' with " +
                           std::to_string(groupViewModel->getPlugins().size()) + " plugins.");
            for (const auto& pluginViewModel : groupViewModel->getPlugins()) {
                if (pluginViewModel->isSelected()) {
                    logMessage(DEBUG,
                               "  Adding selected plugin '" + pluginViewModel->getName() +
                                   "' with " +
                                   std::to_string(pluginViewModel->getPlugin()->files.files.size()) +
                                   " files.");
                    addFiles(allFiles, pluginViewModel->getPlugin()->files.files);
                } else {
                    logMessage(DEBUG,
                               "  Skipping plugin '" + pluginViewModel->getName() +
                                   "' (not selected).");
                }
            }
        }
    }

    // ConditionalInstall files
    for (const auto conditionals = mFomodFile->conditionalFileInstalls; const auto& pattern : conditionals.patterns) {
        //<folder source="CR\Dagi-Raht LL\VLrn_Custom Race - Dagi-Raht LL" destination="" priority="2" />

        if (mConditionTester.testCompositeDependency(mFlagMap, pattern.dependencies)) {
            // also check if the plugins setting these flags are visible. at least one

            addFiles(allFiles, pattern.files.files);
            logMessage(DEBUG,
                       "Conditional install pattern matched; added " +
                           std::to_string(pattern.files.files.size()) + " files.");
        } else {
            logMessage(DEBUG, "Conditional install pattern did not match.");
        }
    }

    // Files will all have a default priority of 0 if not specified, so the order should also be informed by the
    // order they appear within XML. That's why we put conditionalFileInstalls after.
    logMessage(DEBUG, "Sorting " + std::to_string(allFiles.size()) + " files by priority.");
    std::ranges::sort(allFiles, [](const auto& a, const auto& b) {
        return a.priority < b.priority;
    });

    for (const auto& toInstall : allFiles) {
        logMessage(DEBUG, "File to install: " + toInstall.source);
    }
    logMessage(DEBUG,
               "collectFilesToInstall completed with " + std::to_string(allFiles.size()) + " files.");

    return allFiles;
}
