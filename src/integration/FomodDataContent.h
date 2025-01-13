#ifndef FOMODDATACONTENT_H
#define FOMODDATACONTENT_H

#include <moddatacontent.h>
#include <gamebryomoddatacontent.h>

namespace FomodDataContentConstants {
  constexpr int FOMOD_CONTENT_ID = 999; // TODO: Confirm if this is how we should be setting this value. See Gamebryo's enum
}


class FomodDataContent final : public GamebryoModDataContent {
public:
  explicit FomodDataContent(const MOBase::IGameFeatures *gameFeatures) : GamebryoModDataContent(gameFeatures) { }

  std::vector<Content> getAllContents() const override;

  std::vector<int> getContentsFor(std::shared_ptr<const MOBase::IFileTree> fileTree) const override;

};



#endif //FOMODDATACONTENT_H
