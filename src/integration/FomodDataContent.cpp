#include "FomodDataContent.h"
#include <ifiletree.h>
#include <iostream>

std::ostream& operator<<(std::ostream& os, const MOBase::ModDataContent::Content& content) {
  os << content.id() << " " << content.name().toStdString() << " " << content.icon().toStdString();
  return os;
}

template <typename T>
void printVector(const std::vector<T>& vec) {
  for (const auto& element : vec) {
    std::cout << element << std::endl;
  }
  std::cout << std::endl;
}


std::vector<MOBase::ModDataContent::Content> FomodDataContent::getAllContents() const {
  std::vector<Content> contents;
  Content fomodContent = {
    FomodDataContentConstants::FOMOD_CONTENT_ID, "FOMOD", "", true
  };
  contents.emplace_back(fomodContent);
  printVector(contents);
  return contents;
}

std::vector<int> FomodDataContent::getContentsFor(const std::shared_ptr<const MOBase::IFileTree> fileTree) const {
  std::vector<int> contents;

  if (fileTree->find("fomod.json")) {
    contents.push_back(FomodDataContentConstants::FOMOD_CONTENT_ID);
  }
  return contents;
}
