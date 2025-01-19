#ifndef FOMODDATACONTENT_H
#define FOMODDATACONTENT_H

#include <imoinfo.h>
#include <moddatacontent.h>

namespace FomodDataContentConstants {
constexpr int FOMOD_CONTENT_ID = 400400;
}

class FomodDataContent final : public MOBase::ModDataContent {
public:
    explicit
    FomodDataContent(MOBase::IOrganizer* organizer) : mOrganizer(organizer) {}

    [[nodiscard]] std::vector<Content> getAllContents() const override;

    [[nodiscard]] std::vector<int> getContentsFor(std::shared_ptr<const MOBase::IFileTree> fileTree) const override;

private:
    MOBase::IOrganizer* mOrganizer;
};


#endif //FOMODDATACONTENT_H