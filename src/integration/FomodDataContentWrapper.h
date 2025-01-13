// FomodDataContentWrapper.h
#ifndef FOMODDATACONTENTWRAPPER_H
#define FOMODDATACONTENTWRAPPER_H

#include <game_feature.h>
#include "FomodDataContent.h"

class FomodDataContentWrapper : public MOBase::GameFeature {
public:
  FomodDataContentWrapper(std::shared_ptr<FomodDataContent> content)
    : mContent(std::move(content)) {}

  const std::type_info& typeInfo() const override {
    return typeid(FomodDataContentWrapper);
  }

private:
  std::shared_ptr<FomodDataContent> mContent;
};

#endif // FOMODDATACONTENTWRAPPER_H
