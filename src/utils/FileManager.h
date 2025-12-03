#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include <QString>
#include <QStringList>

class FileManager
{
public:
    static QStringList listMarkdownFiles(const QString &dirPath);
    static QString readFile(const QString &filePath);
    static bool writeFile(const QString &filePath, const QString &content);
    static bool ensureDirectory(const QString &dirPath);
};

#endif // FILEMANAGER_H
