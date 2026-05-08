#ifndef TEMPFILEMANAGER_H
#define TEMPFILEMANAGER_H

#include <QString>

class TempFileManager
{
public:
    static QString tempDir();
    static QString filePath(const QString &prefix, const QString &suffix);
    static void cleanup();
};

#endif // TEMPFILEMANAGER_H
