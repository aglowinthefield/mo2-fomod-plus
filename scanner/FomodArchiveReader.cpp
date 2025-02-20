#include "FomodArchiveReader.h"

#include "stringutil.h"
#include "PluginReader.h"

#include <archive.h>
#include <iostream>

const auto PLUGIN_EXTENSIONS = QStringList{ "esp", "esm", "esl" };

bool hasFomodFiles(const std::vector<FileData*>& files)
{
    bool hasModuleXml = false;
    bool hasInfoXml   = false;

    for (const auto* file : files) {
        if (endsWithCaseInsensitive(file->getArchiveFilePath(), StringConstants::FomodFiles::W_MODULE_CONFIG.data())) {
            hasModuleXml = true;
        }
        if (endsWithCaseInsensitive(file->getArchiveFilePath(), StringConstants::FomodFiles::W_INFO_XML.data())) {
            hasInfoXml = true;
        }
    }
    return hasModuleXml && hasInfoXml;
}

std::ostream& operator<<(std::ostream& os, const Archive::Error& error)
{
    switch (error) {
    case Archive::Error::ERROR_NONE:
        os << "No error";
        break;
    case Archive::Error::ERROR_ARCHIVE_NOT_FOUND:
        os << "File not found";
        break;
    case Archive::Error::ERROR_FAILED_TO_OPEN_ARCHIVE:
        os << "Failed to open file";
        break;
    case Archive::Error::ERROR_INVALID_ARCHIVE_FORMAT:
        os << "Invalid archive format";
        break;
    default:
        os << "Unknown error??";
    }
    return os;
}

std::pair<ArchiveScanResult, std::unique_ptr<Archive>> FomodArchiveReader::getArchiveForMod(
    const MOBase::IModInterface* mod) const
{
    const auto downloadsDir         = m_organizer->downloadsPath();
    const auto installationFilePath = mod->installationFile();

    if (installationFilePath.isEmpty()) {
        return std::make_pair(NO_ARCHIVE, nullptr);
    }

    const auto qualifiedInstallerPath = QDir(installationFilePath).isAbsolute()
        ? installationFilePath
        : downloadsDir + "/" + installationFilePath;

    auto archive = CreateArchive();

    if (!archive->isValid()) {
        std::cerr << "[" << mod->name().toStdString() << "] Failed to load the archive module: " << archive->getLastError() << std::endl;
        return std::make_pair(NO_ARCHIVE, nullptr);
    }

    // Open the archive:
    if (!archive->open(qualifiedInstallerPath.toStdWString(), nullptr)) {
        std::cerr << "[" << mod->name().toStdString() << "] Failed to open the archive [" << qualifiedInstallerPath.toStdString() << "]: " << archive->getLastError() << std::endl;
        return std::make_pair(NO_ARCHIVE, nullptr);
    }

    // Mark all files for extraction to their path in the archive:
    if (hasFomodFiles(archive->getFileList())) {
        std::cout << "Found FOMOD files in " << qualifiedInstallerPath.toStdString() << std::endl;
        return std::make_pair(HAS_FOMOD, std::move(archive));
    }

    return std::make_pair(NO_FOMOD, nullptr);
}

ArchiveScanResult FomodArchiveReader::scanInstallationArchive(const MOBase::IModInterface* mod) const
{
    const auto [result, archive] = getArchiveForMod(mod);
    return result;
}

std::vector<std::string> getMastersFromPlugin(const std::wstring& pluginPath)
{
    return readMasters(pluginPath);
}

std::vector<ModWithMasters> FomodArchiveReader::getModsWithMasters(const MOBase::IModInterface* mod) const
{
    std::vector<ModWithMasters> modsWithMasters;

    const auto [result, archive] = getArchiveForMod(mod);
    if (result != HAS_FOMOD) {
        return {};
    }

    std::vector<std::wstring> pluginsToExtract;

    std::cout << "Mod name: " << mod->name().toStdString() << std::endl;
    for (const auto* file : archive->getFileList()) {
        const auto filePath = file->getArchiveFilePath();
        if (std::ranges::any_of(PLUGIN_EXTENSIONS, [filePath](const auto& ext) {
            return endsWithCaseInsensitive(filePath, ext.toStdWString());
        }))
        {
           pluginsToExtract.emplace_back(filePath);
        }
    }

    std::cout << "Files to extract: " << std::endl;
    for (auto toExtract : pluginsToExtract) {
        std::cout << wstringToString(toExtract) << std::endl;
    }

    archive->extract()

    return modsWithMasters;
}