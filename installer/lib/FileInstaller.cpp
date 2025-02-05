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
    const std::vector<std::shared_ptr<StepViewModel> >& steps) : mOrganizer(organizer), mFomodPath(std::move(fomodPath)),
                                                                 mFileTree(fileTree),
                                                                 mFomodFile(std::move(fomodFile)),
                                                                 mFlagMap(flagMap),
                                                                 mConditionTester(organizer), mSteps(steps) {}

std::shared_ptr<IFileTree> FileInstaller::install() const
{
    const auto filesToInstall = collectFilesToInstall();
    std::cout << "Installing " << filesToInstall.size() << " files" << std::endl;

    // update the file tree with the new files
    const std::shared_ptr<IFileTree> installTree = mFileTree->createOrphanTree();

    for (const auto& file : filesToInstall) {

        const auto sourcePath = getQualifiedFilePath(file.source);
        const auto sourceNode = mFileTree->find(QString::fromStdString(sourcePath));
        if (sourceNode == nullptr) {
            std::cerr << "Could not find source: " << file.source << std::endl;
            continue;
        }
        const auto targetPath = QString::fromStdString(file.destination);

        // If it's a folder, copy the contents of the folder, not the folder itself.
        if (sourceNode->isDir()) {
            const auto& tree = sourceNode->astree();
            for (auto it = tree->begin(); it != tree->end(); ++it) {
                // this could be a directory too. ugh
                const auto entry = *it;
                const auto path = targetPath + "/" + entry->name();
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
        stepJson["name"]   = stepViewModel->installStep->name;
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

    notes += "### BEGIN FOMOD NOTES ###\n";

    std::vector<std::string> selectedOptions;
    std::vector<std::string> allOptions;

    for (const auto stepViewModel : mSteps) {
        for (const auto groupViewModel : stepViewModel->getGroups()) {
            for (const auto pluginViewModel : groupViewModel->plugins) {
                if (pluginViewModel->getName() != "None") {
                    allOptions.emplace_back(pluginViewModel->getName());
                    if (pluginViewModel->isSelected()) {
                        selectedOptions.push_back(pluginViewModel->getName());
                    }
                }
            }
        }
    }



    notes += "### END FOMOD NOTES ###\n";

    return notes;
}

// Generic vector appender
void addFiles(std::vector<File>& main, std::vector<File> toAdd)
{
    for (const auto& add : toAdd) {
        std::cout << "Adding file with source: " << add.source << std::endl;
    }
    main.insert(main.end(), toAdd.begin(), toAdd.end());
}

// TODO: Unclear if we're copying. oh well.
std::vector<File> FileInstaller::collectFilesToInstall() const
{
    std::vector<File> filesToInstall;

    // Required files from FOMOD
    const FileList requiredInstallFiles = mFomodFile->requiredInstallFiles;
    addFiles(filesToInstall, requiredInstallFiles.files);

    // ConditionalInstall files
    const auto conditionalInstalls = mFomodFile->conditionalFileInstalls;
    for (const auto& pattern : conditionalInstalls.patterns) {
        if (mConditionTester.testCompositeDependency(mFlagMap, pattern.dependencies)) {
            addFiles(filesToInstall, pattern.files.files);
        }
    }

    // Selected files from visible steps
    for (const auto& stepViewModel : mSteps) {
        if (!mConditionTester.isStepVisible(mFlagMap, stepViewModel->installStep)) {
            continue;
        }
        for (const auto& groupViewModel : stepViewModel->getGroups()) {
            for (const auto& pluginViewModel : groupViewModel->getPlugins()) {
                if (pluginViewModel->isSelected()) {
                    addFiles(filesToInstall, pluginViewModel->getPlugin()->files.files);
                }
            }
        }
    }

    // Sort by priority. I don't think we need to do anything about skipping lower priority mods, but i'm not sure.

    std::ranges::sort(filesToInstall, [](const auto& a, const auto& b) {
        return a.priority < b.priority;
    });

    return filesToInstall;
}