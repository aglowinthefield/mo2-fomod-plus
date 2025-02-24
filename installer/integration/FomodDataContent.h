#ifndef FOMODDATACONTENT_H
#define FOMODDATACONTENT_H

#include <imoinfo.h>
#include <moddatacontent.h>
#include "gamebryomoddatacontent.h"

class FomodDataContent final : public GamebryoModDataContent {
public:
    explicit
    FomodDataContent(MOBase::IOrganizer* organizer);

    [[nodiscard]] std::vector<Content> getAllContents() const override;

    [[nodiscard]] std::vector<int> getContentsFor(std::shared_ptr<const MOBase::IFileTree> fileTree) const override;

protected:
    enum FomodContent {
        CONTENT_FOMOD = CONTENT_NEXT_VALUE
    };

private:
    MOBase::IOrganizer* mOrganizer;
};


#endif //FOMODDATACONTENT_H