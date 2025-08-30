#pragma once

#include <imoinfo.h>
#include <moddatacontent.h>

namespace FomodDataContentConstants {
constexpr int FOMOD_CONTENT_ID = 1000;  // High ID to avoid offset conflicts
}

class FomodDataContent final : public MOBase::ModDataContent {
public:
    explicit FomodDataContent(MOBase::IOrganizer* organizer);

    [[nodiscard]] std::vector<Content> getAllContents() const override;

    [[nodiscard]] std::vector<int> getContentsFor(std::shared_ptr<const MOBase::IFileTree> fileTree) const override;

private:
    MOBase::IOrganizer* mOrganizer;
};