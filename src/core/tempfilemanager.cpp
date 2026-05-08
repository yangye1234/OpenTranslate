#include "tempfilemanager.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QUuid>

namespace {
QString ensureDirectory(const QString &path)
{
    if (path.isEmpty()) {
        return {};
    }

    QDir dir(path);
    if (dir.exists() || dir.mkpath(QStringLiteral("."))) {
        return dir.absolutePath();
    }
    return {};
}

QString appDataTempDir()
{
    const QString baseDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (baseDir.isEmpty()) {
        return {};
    }
    return ensureDirectory(QDir(baseDir).filePath(QStringLiteral("temp")));
}

QString fallbackTempDir()
{
    return ensureDirectory(QDir(QDir::tempPath()).filePath(QStringLiteral("OpenTranslate")));
}
}

QString TempFileManager::tempDir()
{
    const QString appDir = appDataTempDir();
    if (!appDir.isEmpty()) {
        return appDir;
    }
    return fallbackTempDir();
}

QString TempFileManager::filePath(const QString &prefix, const QString &suffix)
{
    const QString directory = tempDir();
    if (directory.isEmpty()) {
        return {};
    }
    return QDir(directory).filePath(QStringLiteral("%1%2%3")
                                       .arg(prefix,
                                            QUuid::createUuid().toString(QUuid::WithoutBraces),
                                            suffix));
}

void TempFileManager::cleanup()
{
    const QString directory = tempDir();
    if (directory.isEmpty()) {
        return;
    }

    QDir dir(directory);
    const QStringList filters {
        QStringLiteral("opentranslate_audio_*"),
        QStringLiteral("opentranslate_ocr_*"),
    };
    const QFileInfoList files = dir.entryInfoList(filters, QDir::Files | QDir::NoSymLinks);
    for (const QFileInfo &file : files) {
        QFile::remove(file.absoluteFilePath());
    }
}
