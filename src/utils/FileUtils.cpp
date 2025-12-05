#include "FileUtils.h"
#include <QTextStream>
#include <QStringConverter>
#include <QDebug>

bool FileUtils::readTextFile(const QString &filePath, QString &content)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "[FileUtils] Failed to open file for reading:" << filePath;
        return false;
    }
    
    // 使用QTextStream确保UTF-8编码（Qt6使用setEncoding）
    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);
    content = stream.readAll();
    file.close();
    
    return true;
}

bool FileUtils::writeTextFile(const QString &filePath, const QString &content)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "[FileUtils] Failed to open file for writing:" << filePath;
        return false;
    }
    
    // 使用QTextStream确保UTF-8编码（Qt6使用setEncoding）
    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);
    stream << content;
    file.close();
    
    return true;
}

bool FileUtils::readJsonFile(const QString &filePath, QByteArray &content)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "[FileUtils] Failed to open JSON file for reading:" << filePath;
        return false;
    }
    
    content = file.readAll();
    file.close();
    
    // JSON文件默认就是UTF-8
    return true;
}

bool FileUtils::writeJsonFile(const QString &filePath, const QByteArray &content)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "[FileUtils] Failed to open JSON file for writing:" << filePath;
        return false;
    }
    
    // 确保写入UTF-8编码的JSON
    file.write(content);
    file.close();
    
    return true;
}
