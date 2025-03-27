#pragma once
#include "stringutil.h"

#include <QDir>
#include <QString>
#include <archive.h>
#include <iostream>
#include <ostream>

inline std::ostream& operator<<(std::ostream& os, const Archive::Error& error)
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

inline bool hasFomodFiles(const std::vector<FileData*>& files)
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

/* This class can do the following:
 * 1.) Detects if an archive has FOMOD files in it (without extracting)
 *      - This is used by the FOMOD scanner to set content flags
 *
 * 2.) It may support extracting ESPs and FOMODs, but this might be better served
 *     by a separate class. Maybe it could return a ModuleConfiguration to use like
 *     the installer.
 */

enum class ScanResult {
    HAS_FOMOD,
    NO_FOMOD,
    NO_ARCHIVE
};

class ArchiveParser {
public:
    static ScanResult scanForFomodFiles(const QString& downloadsPath, const QString& installationFilePath,
        const QString& modName)
    {
        if (installationFilePath.isEmpty()) {
            return ScanResult::NO_ARCHIVE;
        }
        const auto qualifiedInstallerPath = QDir(installationFilePath).isAbsolute()
            ? installationFilePath
            : downloadsPath + "/" + installationFilePath;

        const auto archive = CreateArchive();

        if (!archive->isValid()) {
            logErrorForMod(modName, "Failed to load the archive module ", archive);
            return ScanResult::NO_ARCHIVE;
        }
        if (!archive->open(qualifiedInstallerPath.toStdWString(), nullptr)) {
            logErrorForMod(modName, "Failed to open archive [" + qualifiedInstallerPath + "]", archive);
            return ScanResult::NO_ARCHIVE;
        }

        if (hasFomodFiles(archive->getFileList())) {
            std::cout << "Found FOMOD files in " << qualifiedInstallerPath.toStdString() << std::endl;
            return ScanResult::HAS_FOMOD;
        }
        return ScanResult::NO_FOMOD;
    }

private:
    static void logErrorForMod(const QString& modName, const QString& message, const std::unique_ptr<Archive>& archive)
    {
        std::cerr << "[" << modName.toStdString() << "] " << message.toStdString() << " (" << archive->getLastError() <<
            ")" << std::endl;
    }
};