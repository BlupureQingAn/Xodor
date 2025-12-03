#include "FileManager.h"
#include <QDir>
#include <QFile>
#include <QTextStream>

QStringList FileManager::listMarkdownFiles(const QString &dirPath)
{
    QDir dir(dirPath);
    QStringList filters;
    filters << "*.md" << "*.markdown";
    return dir.entryList(filters, QDir::Files);
}

QString FileManager::readFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString();
    }
    
    QTextStream in(&file);
    QString content = in.readAll();
    file.close();
    return content;
}

bool FileManager::writeFile(const QString &filePath, const QString &content)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }
    
    QTextStream out(&file);
    out << content;
    file.close();
    return true;
}

bool FileManager::ensureDirectory(const QString &dirPath)
{
    QDir dir;
    return dir.mkpath(dirPath);
}
