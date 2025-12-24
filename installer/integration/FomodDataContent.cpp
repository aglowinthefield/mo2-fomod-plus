#include "FomodDataContent.h"

#include <ifiletree.h>

#include "stringutil.h"

#include <iplugingame.h>

FomodDataContent::FomodDataContent(MOBase::IOrganizer* organizer) : mOrganizer(organizer)
{
}

std::vector<MOBase::ModDataContent::Content> FomodDataContent::getAllContents() const
{
    static const std::vector<Content> contents = {
        {FomodDataContentConstants::FOMOD_CONTENT_ID, "FOMOD", ":/fomod/hat", false}
    };

    return contents;
}

// Confirmed working, no need to update
std::vector<int> FomodDataContent::getContentsFor(const std::shared_ptr<const MOBase::IFileTree> fileTree) const
{
    std::vector<int> contents;
    if (!mOrganizer || !fileTree) {
        return contents;
    }

    const auto modList = mOrganizer->modList();
    if (!modList) {
        return contents;
    }

    const auto mod = modList->getMod(fileTree->name());
    if (modHasFomodContent(mod)) {
       contents.emplace_back(FomodDataContentConstants::FOMOD_CONTENT_ID);
    }
    return contents;
}

bool FomodDataContent::modHasFomodContent(const MOBase::IModInterface* mod)
{
    if (!mod) {
        return false;
    }
    const auto pluginName = QString::fromStdString(StringConstants::Plugin::NAME.data());
    const auto fomodMeta = mod->pluginSetting(pluginName, "fomod", 0);
    return fomodMeta != 0;
}
