#pragma once

#include "stringutil.h"

#include <QDir>
#include <QFileInfo>
#include <QString>
#include <QTemporaryDir>
#include <archive.h>
#include <filesystem>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>

struct ExtractionResult {
    bool success = false;
    QString moduleConfigPath;
    std::vector<QString> pluginPaths;
    QString errorMessage;
    std::unique_ptr<QTemporaryDir> tempDir;  // Owns the temp directory lifetime
};

/**
 * Utility class to extract FOMOD data from archives without being in an installer context.
 * Used by the Patch Wizard's rescan functionality.
 */
class ArchiveExtractor {
public:
    using ProgressCallback = std::function<void(const QString& fileName)>;

    /**
     * Extract ModuleConfig.xml and plugin files from an archive.
     * @param archiveFilePath Full path to the archive file
     * @param progressCallback Optional callback for progress updates
     * @return ExtractionResult containing paths to extracted files
     */
    static ExtractionResult extractFomodData(
        const QString& archiveFilePath,
        const ProgressCallback& progressCallback = nullptr)
    {
        ExtractionResult result;
        result.tempDir = std::make_unique<QTemporaryDir>();

        if (!result.tempDir->isValid()) {
            result.errorMessage = "Failed to create temporary directory";
            return result;
        }

        const auto archive = CreateArchive();
        if (!archive->isValid()) {
            result.errorMessage = "Failed to load archive module";
            return result;
        }

        if (!archive->open(archiveFilePath.toStdWString(), nullptr)) {
            result.errorMessage = QString("Failed to open archive (error %1)")
                .arg(static_cast<int>(archive->getLastError()));
            return result;
        }

        // Get file list and mark files for extraction
        const auto& fileList = archive->getFileList();
        QString moduleConfigInArchive;

        for (auto* fileData : fileList) {
            const auto entryPath = QString::fromStdWString(fileData->getArchiveFilePath());

            // Check for ModuleConfig.xml
            if (entryPath.toLower().endsWith("fomod/moduleconfig.xml") ||
                entryPath.toLower().endsWith("fomod\\moduleconfig.xml")) {
                moduleConfigInArchive = entryPath;
                // Set output path relative to output directory for extract()
                fileData->addOutputFilePath(L"ModuleConfig.xml");
                result.moduleConfigPath = result.tempDir->filePath("ModuleConfig.xml");
            }
            // Check for plugin files
            else if (isPluginFile(entryPath)) {
                const auto fileName = QFileInfo(entryPath).fileName();
                const auto relativePath = QString("plugins/") + fileName;
                fileData->addOutputFilePath(relativePath.toStdWString());
                result.pluginPaths.push_back(result.tempDir->filePath(relativePath));
            }
        }

        if (moduleConfigInArchive.isEmpty()) {
            result.errorMessage = "No ModuleConfig.xml found in archive";
            return result;
        }

        // Create plugins subdirectory
        QDir(result.tempDir->path()).mkpath("plugins");

        // Extract the files
        Archive::FileChangeCallback fileChangeCallback = [&progressCallback](
            Archive::FileChangeType, const std::wstring& fileName) {
            if (progressCallback) {
                progressCallback(QString::fromStdWString(fileName));
            }
        };

        Archive::ErrorCallback errorCallback = [&result](const std::wstring& error) {
            result.errorMessage = QString::fromStdWString(error);
        };

        const bool extractSuccess = archive->extract(
            result.tempDir->path().toStdWString(),
            Archive::ProgressCallback{},  // progress callback
            fileChangeCallback,
            errorCallback
        );

        if (!extractSuccess) {
            if (result.errorMessage.isEmpty()) {
                result.errorMessage = "Extraction failed";
            }
            return result;
        }

        // Verify ModuleConfig.xml was extracted
        if (!QFile::exists(result.moduleConfigPath)) {
            result.errorMessage = "ModuleConfig.xml extraction failed";
            return result;
        }

        // Filter to only existing plugin files
        std::vector<QString> existingPlugins;
        for (const auto& path : result.pluginPaths) {
            if (QFile::exists(path)) {
                existingPlugins.push_back(path);
            }
        }
        result.pluginPaths = std::move(existingPlugins);

        result.success = true;
        return result;
    }

    /**
     * Check if an archive contains FOMOD files without extracting.
     * @param archiveFilePath Full path to the archive file
     * @return true if the archive contains fomod/ModuleConfig.xml
     */
    static bool hasFomodFiles(const QString& archiveFilePath)
    {
        const auto archive = CreateArchive();
        if (!archive->isValid() || !archive->open(archiveFilePath.toStdWString(), nullptr)) {
            return false;
        }

        for (const auto* fileData : archive->getFileList()) {
            const auto path = QString::fromStdWString(fileData->getArchiveFilePath());
            if (path.toLower().endsWith("fomod/moduleconfig.xml") ||
                path.toLower().endsWith("fomod\\moduleconfig.xml")) {
                return true;
            }
        }
        return false;
    }
};
