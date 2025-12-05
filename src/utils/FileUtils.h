#ifndef FILEUTILS_H
#define FILEUTILS_H

#include <QString>
#include <QFile>
#include <QTextStream>

/**
 * @brief 文件操作工具类，确保所有文件操作使用UTF-8编码
 */
class FileUtils
{
public:
    /**
     * @brief 读取文本文件（UTF-8编码）
     * @param filePath 文件路径
     * @param content 输出内容
     * @return 是否成功
     */
    static bool readTextFile(const QString &filePath, QString &content);
    
    /**
     * @brief 写入文本文件（UTF-8编码）
     * @param filePath 文件路径
     * @param content 内容
     * @return 是否成功
     */
    static bool writeTextFile(const QString &filePath, const QString &content);
    
    /**
     * @brief 读取JSON文件（UTF-8编码）
     * @param filePath 文件路径
     * @param content 输出内容
     * @return 是否成功
     */
    static bool readJsonFile(const QString &filePath, QByteArray &content);
    
    /**
     * @brief 写入JSON文件（UTF-8编码）
     * @param filePath 文件路径
     * @param content JSON内容
     * @return 是否成功
     */
    static bool writeJsonFile(const QString &filePath, const QByteArray &content);
};

#endif // FILEUTILS_H
