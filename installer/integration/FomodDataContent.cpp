#include "FomodDataContent.h"

#include <ifiletree.h>

#include <iostream>

#include "stringutil.h"

std::ostream& operator<<(std::ostream& os, const MOBase::ModDataContent::Content& content) {
  os << content.id() << " " << content.name().toStdString() << " " << content.icon().toStdString();
  return os;
}

std::vector<MOBase::ModDataContent::Content> FomodDataContent::getAllContents() const {
  std::vector<Content> contents;
  Content fomodContent = {
    FomodDataContentConstants::FOMOD_CONTENT_ID, "FOMOD", ":/fomod/hat", false
  };
  contents.emplace_back(fomodContent);
  return contents;
}

std::vector<int> FomodDataContent::getContentsFor(const std::shared_ptr<const MOBase::IFileTree> fileTree) const {
  std::vector<int> contents;

  const auto modName = fileTree->name();
  const auto mod = mOrganizer->modList()->getMod(modName);

  if (const auto fomodMeta = mod->pluginSetting(QString::fromStdString(StringConstants::Plugin::NAME.data()), "fomod", 0); fomodMeta != 0) {
    contents.push_back(FomodDataContentConstants::FOMOD_CONTENT_ID);
  }
  return contents;
}
