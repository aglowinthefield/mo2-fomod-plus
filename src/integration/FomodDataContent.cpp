#include "FomodDataContent.h"
#include <ifiletree.h>

std::vector<MOBase::ModDataContent::Content> FomodDataContent::getAllContents() const {
  return {Content(FomodDataContentConstants::FOMOD_CONTENT_ID, "FOMOD", ":/resources/contents/fomod.png")};
}

std::vector<int> FomodDataContent::getContentsFor(const std::shared_ptr<const MOBase::IFileTree> fileTree) const {
  std::vector<int> contents;
  if (fileTree->find("fomod.json")) {
    contents.push_back(FomodDataContentConstants::FOMOD_CONTENT_ID);
  }
  return contents;

}
