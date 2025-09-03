#pragma once

#include <uibase/game_features/moddatacontent.h>
#include <uibase/imoinfo.h>

namespace FomodDataContentConstants {
constexpr int FOMOD_CONTENT_ID = 400400;
}

class FomodDataContent final : public MOBase::ModDataContent {
  public:
    explicit FomodDataContent(MOBase::IOrganizer* organizer);

    [[nodiscard]] std::vector<Content> getAllContents() const override;

    [[nodiscard]] std::vector<int> getContentsFor(std::shared_ptr<const MOBase::IFileTree> fileTree) const override;

  private:
    MOBase::IOrganizer* mOrganizer;
    static bool modHasFomodContent(const MOBase::IModInterface* mod);
};
