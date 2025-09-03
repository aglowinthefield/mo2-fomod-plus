#include "FomodDataContent.h"

#include <fomod_plus_shared/stringutil.h>
#include <uibase/game_features/moddatacontent.h>
#include <uibase/ifiletree.h>
#include <uibase/iplugingame.h>

FomodDataContent::FomodDataContent(MOBase::IOrganizer* organizer)
    : mOrganizer(organizer)
{
}

std::vector<MOBase::ModDataContent::Content> FomodDataContent::getAllContents() const
{
    static const std::vector<Content> contents
        = { { FomodDataContentConstants::FOMOD_CONTENT_ID, "FOMOD", ":/fomod/hat", false } };
    return contents;
}

// Confirmed working, no need to update
std::vector<int> FomodDataContent::getContentsFor(const std::shared_ptr<const MOBase::IFileTree> fileTree) const
{
    std::vector<int> contents;
    if (modHasFomodContent(mOrganizer->modList()->getMod(fileTree->name()))) {
        contents.emplace_back(FomodDataContentConstants::FOMOD_CONTENT_ID);
    }
    return contents;
}

bool FomodDataContent::modHasFomodContent(const MOBase::IModInterface* mod)
{
    const auto pluginName = QString::fromStdString(StringConstants::Plugin::NAME.data());
    if (mod->isSeparator() || mod->isForeign() || mod->isBackup() || mod->isOverwrite()) {
        return false;
    }
    const auto fomodMeta = mod->pluginSetting(pluginName, "fomod", 0);
    return fomodMeta != 0;
}
