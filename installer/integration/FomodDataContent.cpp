#include "FomodDataContent.h"

#include <ifiletree.h>

#include "stringutil.h"

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

std::vector<int> FomodDataContent::getContentsFor(const std::shared_ptr<const MOBase::IFileTree> fileTree) const
{
    std::vector<int> contents;

    const auto mod = mOrganizer->modList()->getMod(fileTree->name());

    if (const auto fomodMeta = mod->pluginSetting(QString::fromStdString(StringConstants::Plugin::NAME.data()), "fomod", 0); fomodMeta != 0) {
        contents.push_back(FomodDataContentConstants::FOMOD_CONTENT_ID);
    }

    return contents;
}