#include "ImportRuleManager.h"
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QDebug>

const QString ImportRuleManager::CONFIG_DIR = "data/config";

bool ImportRuleManager::saveImportRule(const QString &bankName, const QJsonObject &rule)
{
    if (bankName.isEmpty()) {
        qWarning() << "[ImportRuleManager] 题库名称为空，无法保存规则";
        return false;
    }
    
    // 确保config目录存在
    QDir dir;
    if (!dir.mkpath(CONFIG_DIR)) {
        qWarning() << "[ImportRuleManager] 无法创建config目录:" << CONFIG_DIR;
        return false;
    }
    
    QString filePath = getRulePath(bankName);
    QFile file(filePath);
    
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "[ImportRuleManager] 无法打开文件进行写入:" << filePath;
        return false;
    }
    
    QJsonDocument doc(rule);
    qint64 bytesWritten = file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    
    if (bytesWritten == -1) {
        qWarning() << "[ImportRuleManager] 写入文件失败:" << filePath;
        return false;
    }
    
    qDebug() << "[ImportRuleManager] 规则文件保存成功:" << filePath;
    return true;
}

QJsonObject ImportRuleManager::loadImportRule(const QString &bankName)
{
    if (bankName.isEmpty()) {
        qWarning() << "[ImportRuleManager] 题库名称为空，无法读取规则";
        return QJsonObject();
    }
    
    QString filePath = getRulePath(bankName);
    QFile file(filePath);
    
    if (!file.exists()) {
        qDebug() << "[ImportRuleManager] 规则文件不存在:" << filePath;
        return QJsonObject();
    }
    
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "[ImportRuleManager] 无法打开文件进行读取:" << filePath;
        return QJsonObject();
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) {
        qWarning() << "[ImportRuleManager] 规则文件格式错误:" << filePath;
        return QJsonObject();
    }
    
    qDebug() << "[ImportRuleManager] 规则文件读取成功:" << filePath;
    return doc.object();
}

bool ImportRuleManager::hasImportRule(const QString &bankName)
{
    if (bankName.isEmpty()) {
        return false;
    }
    
    QString filePath = getRulePath(bankName);
    return QFile::exists(filePath);
}

bool ImportRuleManager::deleteImportRule(const QString &bankName)
{
    if (bankName.isEmpty()) {
        qWarning() << "[ImportRuleManager] 题库名称为空，无法删除规则";
        return false;
    }
    
    QString filePath = getRulePath(bankName);
    
    if (!QFile::exists(filePath)) {
        qDebug() << "[ImportRuleManager] 规则文件不存在，无需删除:" << filePath;
        return true;  // 文件不存在也算成功
    }
    
    if (QFile::remove(filePath)) {
        qDebug() << "[ImportRuleManager] 规则文件删除成功:" << filePath;
        return true;
    } else {
        qWarning() << "[ImportRuleManager] 规则文件删除失败:" << filePath;
        return false;
    }
}

QString ImportRuleManager::getRulePath(const QString &bankName)
{
    // 文件命名格式：{bankName}_parse_rule.json
    return QString("%1/%2_parse_rule.json").arg(CONFIG_DIR).arg(bankName);
}
