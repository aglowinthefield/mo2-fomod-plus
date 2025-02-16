#include "FileInstaller.h"

#include <fstream>
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
                                                                 mConditionTester(organizer), mSteps(steps) {}

std::shared_ptr<IFileTree> FileInstaller::install() const
{
    const auto filesToInstall = collectFilesToInstall();
    logMessage(INFO, "Installing " + std::to_string(filesToInstall.size()) + " files");
    logMessage(INFO, "FlagMap");
    logMessage(INFO, mFlagMap->toString());

    // update the file tree with the new files
    const std::shared_ptr<IFileTree> installTree = mFileTree->createOrphanTree();

    for (const auto& file : filesToInstall) {

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
        if (sourceNode->isDir()) { // TODO: Check if target path is literally undefined/null
            const auto& tree = sourceNode->astree();
            for (auto it = tree->begin(); it != tree->end(); ++it) {
                const auto entry = *it;
                const auto path  = (targetPath.isEmpty()) ? entry->name() : targetPath + "/" + entry->name();
                installTree->copy(entry, path, IFileTree::InsertPolicy::MERGE);
            }
        } else {
            installTree->copy(sourceNode, targetPath, IFileTree::InsertPolicy::MERGE);
        }
    }

    // This file will be written by the InstallationManager later.
    const auto jsonFilePath = "fomod.json";
    installTree->addFile(QString::fromStdString(jsonFilePath), true);

    return installTree;
}

nlohmann::json FileInstaller::generateFomodJson() const
{
    nlohmann::json fomodJson;

    fomodJson["steps"] = nlohmann::json::array();
    for (const auto& stepViewModel : mSteps) {
        auto stepJson      = nlohmann::json::object();
        stepJson["name"]   = stepViewModel->getName();
        stepJson["groups"] = nlohmann::json::array();

        for (const auto& groupViewModel : stepViewModel->getGroups()) {
            auto groupJson    = nlohmann::json::object();
            groupJson["name"] = groupViewModel->getName();
            auto pluginArray  = nlohmann::json::array();

            for (const auto& pluginViewModel : groupViewModel->getPlugins()) {
                if (pluginViewModel->isSelected()) {
                    pluginArray.emplace_back(pluginViewModel->getName());
                }
            }
            groupJson["plugins"] = pluginArray;
            stepJson["groups"].emplace_back(groupJson);
        }
        fomodJson["steps"].emplace_back(stepJson);
    }
    return fomodJson;
}

std::string FileInstaller::getQualifiedFilePath(const std::string& treePath) const
{
    // We need to prepend the fomod path to whatever source we reference. Guess we're passing that path around.
    return mFomodPath.toStdString() + "/" + treePath;
}

QString FileInstaller::createInstallationNotes() const
{
    QString notes = "";
    std::vector<std::string> hasPatchFor;
    std::vector<std::string> installedPatchFor;
    std::vector<std::string> notInstalledPatchFor;

    for (const auto stepViewModel : mSteps) {
        for (const auto groupViewModel : stepViewModel->getGroups()) {
            for (const auto pluginViewModel : groupViewModel->getPlugins()) {

                const auto patterns = pluginViewModel->getPlugin()->typeDescriptor.dependencyType.patterns.patterns;
                const auto fileNames = collectPositiveFileNamesFromDependencyPatterns(patterns);

                for (auto fileName : fileNames) {
                    hasPatchFor.emplace_back("hasPatchFor:" + fileName);
                    if (pluginViewModel->isSelected()) {
                        installedPatchFor.emplace_back("installedPatchFor:" + fileName);
                    } else {
                        notInstalledPatchFor.emplace_back("notInstalledPatchFor:" + fileName);
                    }
                }
            }
        }
    }

    notes += "BEGIN FOMOD NOTES\n";
    for (auto patchFor : hasPatchFor) {
        notes += patchFor + "\n";
    }
    notes += "\n";
    for (auto patchFor : installedPatchFor) {
        notes += patchFor + "\n";
    }
    notes += "\n";
    for (auto patchFor : notInstalledPatchFor) {
        notes += patchFor + "\n";
    }
    notes += "\nEND FOMOD NOTES\n";
    return notes;
}

std::vector<std::string> FileInstaller::collectPositiveFileNamesFromDependencyPatterns(std::vector<DependencyPattern> patterns)
{
    std::vector<std::string> usableFileDependencyPluginNames = {};

    for (const auto pattern : patterns) {
        if (pattern.type == PluginTypeEnum::NotUsable) {
            continue;
        }

        if (pattern.dependencies.fileDependencies.size() == 0 && pattern.dependencies.nestedDependencies.size() == 0) {
            continue;
        }

        const auto fileDependencies   = pattern.dependencies.fileDependencies;
        const auto nestedDependencies = pattern.dependencies.nestedDependencies;

        for (const auto fileDependency : fileDependencies) {
            if (fileDependency.state != FileDependencyTypeEnum::Active) {
                continue;
            }
            usableFileDependencyPluginNames.emplace_back(fileDependency.file);
        }

        for (const auto nestedDependency : nestedDependencies) {
            for (const auto fileDependency : nestedDependency.fileDependencies) {
                if (fileDependency.state != FileDependencyTypeEnum::Active) {
                    continue;
                }
                usableFileDependencyPluginNames.emplace_back(fileDependency.file);
            }
        }
        // Not handling twice-nested dependencies now. IDK if that's even feasible.
    }

    return usableFileDependencyPluginNames;
}

// Generic vector appender
void FileInstaller::addFiles(std::vector<File>& main, std::vector<File> toAdd) const
{
    for (const auto& add : toAdd) {
        logMessage(INFO, "Adding file with source: " + add.source);
    }
    main.insert(main.end(), toAdd.begin(), toAdd.end());
}

// TODO: Unclear if we're copying. oh well.
// TODO: Rebuild flagmap and step indeces before installing
std::vector<File> FileInstaller::collectFilesToInstall() const
{
    std::vector<File> allFiles;

    // Required files from FOMOD
    const FileList requiredInstallFiles = mFomodFile->requiredInstallFiles;
    addFiles(allFiles, requiredInstallFiles.files);

    // Selected files from visible steps
    for (const auto& stepViewModel : mSteps) {
        if (!mConditionTester.testCompositeDependency(mFlagMap, stepViewModel->getVisibilityConditions())) {
            continue;
        }
        for (const auto& groupViewModel : stepViewModel->getGroups()) {
            for (const auto& pluginViewModel : groupViewModel->getPlugins()) {
                if (pluginViewModel->isSelected()) {
                    addFiles(allFiles, pluginViewModel->getPlugin()->files.files);
                }
            }
        }
    }

    // ConditionalInstall files
    for (const auto conditionals = mFomodFile->conditionalFileInstalls; const auto& pattern : conditionals.patterns) {
        //<folder source="CR\Dagi-Raht LL\VLrn_Custom Race - Dagi-Raht LL" destination="" priority="2" />

        if (pattern.files.files.front().source == "CR\\Dagi-Raht LL\\VLrn_Custom Race - Dagi-Raht LL") {
            logMessage(DEBUG, "WHY IS THIS HERE");
        }
        if (mConditionTester.testCompositeDependency(mFlagMap, pattern.dependencies)) {
            // also check if the plugins setting these flags are visible. at least one

            addFiles(allFiles, pattern.files.files);
        }
    }

    // Files will all have a default priority of 0 if not specified, so the order should also be informed by the
    // order they appear within XML. That's why we put conditionalFileInstalls after.
    std::ranges::sort(allFiles, [](const auto& a, const auto& b) {
        return a.priority < b.priority;
    });

    for (auto toInstall : allFiles) {
        logMessage(DEBUG, "File to install: " + toInstall.source);
    }

    return allFiles;
}