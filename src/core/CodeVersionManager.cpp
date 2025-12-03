#include "CodeVersionManager.h"
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QDebug>

CodeVersionManager::CodeVersionManager(QObject *parent)
    : QObject(parent)
{
}

QString CodeVersionManager::generateVersionId() const
{
    return QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss_zzz");
}

QString CodeVersionManager::getVersionsRootDir() const
{
    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QString versionsPath = dataPath + "/CodeVersions";
    
    QDir dir;
    if (!dir.exists(versionsPath)) {
        dir.mkpath(versionsPath);
    }
    
    return versionsPath;
}

QString CodeVersionManager::getVersionsDir(const QString &questionId) const
{
    QString dir = getVersionsRootDir() + "/" + questionId;
    
    QDir qdir;
    if (!qdir.exists(dir)) {
        qdir.mkpath(dir);
    }
    
    return dir;
}

QString CodeVersionManager::getVersionFilePath(const QString &questionId, const QString &versionId) const
{
    return getVersionsDir(questionId) + "/" + versionId + ".json";
}

int CodeVersionManager::countLines(const QString &code) const
{
    if (code.isEmpty()) return 0;
    return code.count('\n') + 1;
}

QString CodeVersionManager::saveVersion(const QString &questionId, const QString &code, 
                                       bool testPassed, const QString &testResult)
{
    CodeVersion version;
    version.versionId = generateVersionId();
    version.questionId = questionId;
    version.code = code;
    version.timestamp = QDateTime::currentDateTime();
    version.lineCount = countLines(code);
    version.testPassed = testPassed;
    version.testResult = testResult;
    
    QString filePath = getVersionFilePath(questionId, version.versionId);
    
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Failed to save code version:" << filePath;
        return QString();
    }
    
    file.write(version.toJson().toUtf8());
    file.close();
    
    qDebug() << "Code version saved:" << version.versionId << "for question:" << questionId;
    
    emit versionSaved(questionId, version.versionId);
    
    // 自动清理旧版本
    cleanOldVersions(questionId);
    
    return version.versionId;
}

QVector<CodeVersion> CodeVersionManager::getVersions(const QString &questionId) const
{
    QVector<CodeVersion> versions;
    
    QString versionsDir = getVersionsDir(questionId);
    QDir dir(versionsDir);
    
    QStringList filters;
    filters << "*.json";
    
    QFileInfoList files = dir.entryInfoList(filters, QDir::Files, QDir::Time | QDir::Reversed);
    
    for (const QFileInfo &fileInfo : files) {
        QFile file(fileInfo.absoluteFilePath());
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString json = file.readAll();
            file.close();
            
            CodeVersion version = CodeVersion::fromJson(json);
            if (!version.versionId.isEmpty()) {
                versions.append(version);
            }
        }
    }
    
    return versions;
}

CodeVersion CodeVersionManager::getVersion(const QString &questionId, const QString &versionId) const
{
    QString filePath = getVersionFilePath(questionId, versionId);
    
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to load code version:" << filePath;
        return CodeVersion();
    }
    
    QString json = file.readAll();
    file.close();
    
    return CodeVersion::fromJson(json);
}

CodeVersion CodeVersionManager::getLatestVersion(const QString &questionId) const
{
    QVector<CodeVersion> versions = getVersions(questionId);
    
    if (versions.isEmpty()) {
        return CodeVersion();
    }
    
    return versions.first();  // 已按时间倒序排列
}

bool CodeVersionManager::deleteVersion(const QString &questionId, const QString &versionId)
{
    QString filePath = getVersionFilePath(questionId, versionId);
    
    if (QFile::remove(filePath)) {
        qDebug() << "Code version deleted:" << versionId;
        emit versionDeleted(questionId, versionId);
        return true;
    }
    
    qWarning() << "Failed to delete code version:" << filePath;
    return false;
}

void CodeVersionManager::cleanOldVersions(const QString &questionId, int keepCount)
{
    QVector<CodeVersion> versions = getVersions(questionId);
    
    if (versions.size() <= keepCount) {
        return;  // 不需要清理
    }
    
    // 删除超出保留数量的旧版本
    for (int i = keepCount; i < versions.size(); ++i) {
        deleteVersion(questionId, versions[i].versionId);
    }
    
    qDebug() << "Cleaned old versions for question:" << questionId 
             << "Kept:" << keepCount << "Deleted:" << (versions.size() - keepCount);
}

int CodeVersionManager::getVersionCount(const QString &questionId) const
{
    return getVersions(questionId).size();
}

QString CodeVersion::toJson() const
{
    QJsonObject obj;
    obj["versionId"] = versionId;
    obj["questionId"] = questionId;
    obj["code"] = code;
    obj["timestamp"] = timestamp.toString(Qt::ISODate);
    obj["lineCount"] = lineCount;
    obj["testPassed"] = testPassed;
    obj["testResult"] = testResult;
    
    return QString(QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

CodeVersion CodeVersion::fromJson(const QString &json)
{
    CodeVersion version;
    
    QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
    if (!doc.isObject()) {
        return version;
    }
    
    QJsonObject obj = doc.object();
    version.versionId = obj["versionId"].toString();
    version.questionId = obj["questionId"].toString();
    version.code = obj["code"].toString();
    version.timestamp = QDateTime::fromString(obj["timestamp"].toString(), Qt::ISODate);
    version.lineCount = obj["lineCount"].toInt();
    version.testPassed = obj["testPassed"].toBool();
    version.testResult = obj["testResult"].toString();
    
    return version;
}
