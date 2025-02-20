#ifndef FOMODARCHIVEREADER_H
#define FOMODARCHIVEREADER_H
#include <archive.h>
#include <complex.h>
#include <imoinfo.h>
#include <string>
#include <vector>

enum ArchiveScanResult {
    HAS_FOMOD,
    NO_FOMOD,
    NO_ARCHIVE
};

struct ModWithMasters {
    std::string modName;
    std::vector<std::string> masters;
};

class FomodArchiveReader final {
public:
    explicit FomodArchiveReader(MOBase::IOrganizer* organizer): m_organizer(organizer) {}

    std::pair<ArchiveScanResult, std::unique_ptr<Archive>> getArchiveForMod(
        const MOBase::IModInterface* mod) const;

    ArchiveScanResult scanInstallationArchive(const MOBase::IModInterface* mod) const;

    [[nodiscard]] std::vector<ModWithMasters> getModsWithMasters(const MOBase::IModInterface *mod) const;

private:
    MOBase::IOrganizer* m_organizer;
};

#endif //FOMODARCHIVEREADER_H