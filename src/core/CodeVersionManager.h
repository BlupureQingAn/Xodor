#ifndef CODEVERSIONMANAGER_H
#define CODEVERSIONMANAGER_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QDateTime>

// 代码版本信息
struct CodeVersion {
    QString versionId;        // 唯一ID（时间戳）
    QString questionId;       // 题目ID
    QString code;             // 代码内容
    QDateTime timestamp;      // 保存时间
    int lineCount;            // 代码行数
    bool testPassed;          // 是否通过测试
    QString testResult;       // 测试结果摘要（如 "5/5"）
    
    QString toJson() const;
    static CodeVersion fromJson(const QString &json);
};

// 代码版本管理器
class CodeVersionManager : public QObject
{
    Q_OBJECT
public:
    explicit CodeVersionManager(QObject *parent = nullptr);
    
    // 保存新版本
    QString saveVersion(const QString &questionId, const QString &code, 
                       bool testPassed = false, const QString &testResult = "");
    
    // 获取版本列表（按时间倒序）
    QVector<CodeVersion> getVersions(const QString &questionId) const;
    
    // 获取指定版本
    CodeVersion getVersion(const QString &questionId, const QString &versionId) const;
    
    // 获取最新版本
    CodeVersion getLatestVersion(const QString &questionId) const;
    
    // 删除指定版本
    bool deleteVersion(const QString &questionId, const QString &versionId);
    
    // 删除旧版本（保留最近N个）
    void cleanOldVersions(const QString &questionId, int keepCount = 10);
    
    // 获取版本数量
    int getVersionCount(const QString &questionId) const;
    
signals:
    void versionSaved(const QString &questionId, const QString &versionId);
    void versionDeleted(const QString &questionId, const QString &versionId);
    
private:
    QString generateVersionId() const;
    QString getVersionsDir(const QString &questionId) const;
    QString getVersionFilePath(const QString &questionId, const QString &versionId) const;
    QString getVersionsRootDir() const;
    int countLines(const QString &code) const;
};

#endif // CODEVERSIONMANAGER_H
