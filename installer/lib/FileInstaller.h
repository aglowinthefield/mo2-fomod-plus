#pragma once

#include <ifiletree.h>
#include <nlohmann/json.hpp>

#include "ConditionTester.h"
#include "FlagMap.h"
#include "Logger.h"
#include "xml/ModuleConfiguration.h"


/* This is what legacy fomodInstaller does:
        modName.update(dialog.getName(), GUESS_USER);
        return dialog.updateTree(tree);

   This is a link to the legacy updateTree method:
    https://github.com/ModOrganizer2/modorganizer-installer_fomod/blob/fc263f2d923c704b4853c11ed4f8b8cf3920f30d/src/fomodinstallerdialog.cpp#L546

*/

// Things this should do:
// - Copy all requiredInstall files
// - Copy all conditionalInstall files
// - Copy all selected files from steps that were visible to the user at the point of install

using namespace MOBase;

using FileGlobalIndex = int;
using FileDescriptor = std::pair<File, FileGlobalIndex>;

class StepViewModel;

class FileInstaller {
public:
    FileInstaller(
        IOrganizer* organizer,
        QString fomodPath,
        const std::shared_ptr<IFileTree>& fileTree,
        std::unique_ptr<ModuleConfiguration> fomodFile,
        const std::shared_ptr<FlagMap>& flagMap,
        const std::vector<std::shared_ptr<StepViewModel> >& steps);

    std::shared_ptr<IFileTree> install() const;

    /**
     * @brief Create a 'fomod.json' file to add to the base of the installTree. Functionally similar to MO2's meta.ini.
     *
     * Until there's more utility in the JSON structure itself, it will simply be of this format:
     * @code
     * {
     *  "steps": [
     *    {
     *      "name": "Step 1",
     *      "groups": [
     *        {
     *          "name": "Group 1",
     *          "plugins: [
     *            "Plugin1.esp",
     *            "Plugin2.esp"
     *          ]
     *        }
     *      ]
     *    }
     *  ]
     * }
     * @endcode
     *
     * @return nhlohmann::json
     */
    nlohmann::json generateFomodJson() const;

    std::string getQualifiedFilePath(const std::string& treePath) const;

    std::vector<std::string> collectPositiveFileNamesFromDependencyPatterns(const std::vector<DependencyPattern> &patterns);

    void addFiles(std::vector<File>& main, std::vector<File> toAdd) const;

private:
    IOrganizer* mOrganizer;
    Logger& log = Logger::getInstance();
    QString mFomodPath;
    std::shared_ptr<IFileTree> mFileTree;
    std::unique_ptr<ModuleConfiguration> mFomodFile;
    std::shared_ptr<FlagMap> mFlagMap;
    ConditionTester mConditionTester;
    std::vector<std::shared_ptr<StepViewModel> > mSteps; // TODO: Maybe this is nasty. Idk.

    std::vector<File> collectFilesToInstall() const;

    void logMessage(LogLevel level, const std::string& message) const
    {
        log.logMessage(level, "[INSTALLER] " + message);
    }
};
