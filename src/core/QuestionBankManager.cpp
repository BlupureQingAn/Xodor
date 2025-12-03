#include "QuestionBankManager.h"
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QUuid>
#include <QDebug>

QString QuestionBankInfo::toJson() const
{
    QJsonObject obj;
    obj["id"] = id;
    obj["name"] = name;
    obj["path"] = path;
    obj["originalPath"] = originalPath;
    obj["questionCount"] = questionCount;
    obj["importTime"] = importTime.toString(Qt::ISODate);
    obj["lastAccessTime"] = lastAccessTime.toString(Qt::ISODate);
    obj["isAIParsed"] = isAIParsed;
    obj["type"] = static_cast<int>(type);
    
    return QString(QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

QuestionBankInfo QuestionBankInfo::fromJson(const QString &json)
{
    QuestionBankInfo info;
    QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
    
    if (!doc.isObject()) return info;
    
    QJsonObject obj = doc.object();
    info.id = obj["id"].toString();
    info.name = obj["name"].toString();
    info.path = obj["path"].toString();
    info.originalPath = obj["originalPath"].toString();
    info.questionCount = obj["questionCount"].toInt();
    info.importTime = QDateTime::fromString(obj["importTime"].toString(), Qt::ISODate);
    info.lastAccessTime = QDateTime::fromString(obj["lastAccessTime"].toString(), Qt::ISODate);
    info.isAIParsed = obj["isAIParsed"].toBool();
    info.type = static_cast<QuestionBankType>(obj["type"].toInt(1)); // 默认为 Processed
    
    return info;
}

QuestionBankManager& QuestionBankManager::instance()
{
    static QuestionBankManager instance;
    return instance;
}

QuestionBankManager::QuestionBankManager(QObject *parent)
    : QObject(parent)
{
    load();
}

QuestionBankManager::~QuestionBankManager()
{
    save();
}

QString QuestionBankManager::generateBankId() const
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

QString QuestionBankManager::getInternalStorageRoot() const
{
    // 使用项目根目录的data文件夹
    QString banksPath = "data";
    
    QDir dir;
    if (!dir.exists(banksPath)) {
        dir.mkpath(banksPath);
    }
    
    return banksPath;
}

QString QuestionBankManager::getOriginalBanksRoot() const
{
    // 原始题库目录
    QString root = "data/原始题库";
    QDir dir;
    if (!dir.exists(root)) {
        dir.mkpath(root);
    }
    return root;
}

QString QuestionBankManager::getProcessedBanksRoot() const
{
    // 基础题库目录（AI解析后的题库）
    QString root = "data/基础题库";
    QDir dir;
    if (!dir.exists(root)) {
        dir.mkpath(root);
    }
    return root;
}

QString QuestionBankManager::getMockBanksRoot() const
{
    QString root = getInternalStorageRoot() + "/mock_banks";
    QDir dir;
    if (!dir.exists(root)) {
        dir.mkpath(root);
    }
    return root;
}

QString QuestionBankManager::getBankPath(const QString &bankId, QuestionBankType type) const
{
    QString root;
    switch (type) {
        case QuestionBankType::Original:
            root = getOriginalBanksRoot();
            break;
        case QuestionBankType::Processed:
            root = getProcessedBanksRoot();
            break;
        case QuestionBankType::Mock:
            root = getMockBanksRoot();
            break;
    }
    return root + "/" + bankId;
}

QString QuestionBankManager::getBankStoragePath(const QString &bankId) const
{
    return getInternalStorageRoot() + "/" + bankId;
}

QString QuestionBankManager::getConfigFilePath() const
{
    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(dataPath);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    return dataPath + "/question_banks.json";
}

bool QuestionBankManager::copyDirectory(const QString &source, const QString &destination)
{
    QDir sourceDir(source);
    if (!sourceDir.exists()) {
        qWarning() << "Source directory does not exist:" << source;
        return false;
    }
    
    QDir destDir(destination);
    if (!destDir.exists()) {
        if (!destDir.mkpath(".")) {
            qWarning() << "Failed to create destination directory:" << destination;
            return false;
        }
    }
    
    // 复制所有文件
    QFileInfoList files = sourceDir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot);
    for (const QFileInfo &fileInfo : files) {
        QString srcPath = fileInfo.absoluteFilePath();
        QString dstPath = destination + "/" + fileInfo.fileName();
        
        if (!QFile::copy(srcPath, dstPath)) {
            qWarning() << "Failed to copy file:" << srcPath << "to" << dstPath;
            return false;
        }
    }
    
    // 递归复制子目录
    QFileInfoList dirs = sourceDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QFileInfo &dirInfo : dirs) {
        QString srcPath = dirInfo.absoluteFilePath();
        QString dstPath = destination + "/" + dirInfo.fileName();
        
        if (!copyDirectory(srcPath, dstPath)) {
            return false;
        }
    }
    
    return true;
}

QString QuestionBankManager::importQuestionBank(const QString &sourcePath, const QString &name, bool isAIParsed)
{
    // 生成唯一ID
    QString bankId = generateBankId();
    
    // 创建内部存储路径
    QString internalPath = getBankStoragePath(bankId);
    
    // 复制题库文件
    qDebug() << "Copying question bank from" << sourcePath << "to" << internalPath;
    if (!copyDirectory(sourcePath, internalPath)) {
        qWarning() << "Failed to copy question bank";
        return QString();
    }
    
    // 创建题库信息
    QuestionBankInfo info;
    info.id = bankId;
    info.name = name;
    info.path = internalPath;
    info.originalPath = sourcePath;
    info.questionCount = 0;  // 稍后更新
    info.importTime = QDateTime::currentDateTime();
    info.lastAccessTime = info.importTime;
    info.isAIParsed = isAIParsed;
    info.type = QuestionBankType::Processed;  // 默认为基础题库
    
    // 添加到列表
    m_banks.append(info);
    
    // 设置为当前题库
    m_currentBankId = bankId;
    
    // 保存配置
    save();
    
    emit bankListChanged();
    emit currentBankChanged(bankId);
    
    qDebug() << "Question bank imported successfully:" << bankId;
    
    return bankId;
}

bool QuestionBankManager::deleteQuestionBank(const QString &bankId)
{
    // 查找题库
    int index = -1;
    for (int i = 0; i < m_banks.size(); ++i) {
        if (m_banks[i].id == bankId) {
            index = i;
            break;
        }
    }
    
    if (index < 0) {
        qWarning() << "Question bank not found:" << bankId;
        return false;
    }
    
    // 删除内部文件夹
    QString bankPath = getBankStoragePath(bankId);
    QDir dir(bankPath);
    if (dir.exists()) {
        if (!dir.removeRecursively()) {
            qWarning() << "Failed to delete question bank directory:" << bankPath;
            return false;
        }
    }
    
    // 从列表中移除
    m_banks.removeAt(index);
    
    // 如果删除的是当前题库，清空当前题库
    if (m_currentBankId == bankId) {
        m_currentBankId.clear();
        emit currentBankChanged(QString());
    }
    
    // 保存配置
    save();
    
    emit bankListChanged();
    
    qDebug() << "Question bank deleted:" << bankId;
    
    return true;
}

bool QuestionBankManager::renameQuestionBank(const QString &bankId, const QString &newName)
{
    for (auto &bank : m_banks) {
        if (bank.id == bankId) {
            bank.name = newName;
            save();
            emit bankListChanged();
            return true;
        }
    }
    return false;
}

QVector<QuestionBankInfo> QuestionBankManager::getAllBanks() const
{
    return m_banks;
}

QVector<QuestionBankInfo> QuestionBankManager::getBanksByType(QuestionBankType type) const
{
    QVector<QuestionBankInfo> result;
    for (const auto &bank : m_banks) {
        if (bank.type == type) {
            result.append(bank);
        }
    }
    return result;
}

QuestionBankInfo QuestionBankManager::getBankInfo(const QString &bankId) const
{
    for (const auto &bank : m_banks) {
        if (bank.id == bankId) {
            return bank;
        }
    }
    return QuestionBankInfo();
}

bool QuestionBankManager::switchToBank(const QString &bankId)
{
    // 检查题库是否存在
    bool found = false;
    for (auto &bank : m_banks) {
        if (bank.id == bankId) {
            found = true;
            bank.lastAccessTime = QDateTime::currentDateTime();
            break;
        }
    }
    
    if (!found) {
        qWarning() << "Question bank not found:" << bankId;
        return false;
    }
    
    m_currentBankId = bankId;
    save();
    
    emit currentBankChanged(bankId);
    
    return true;
}

void QuestionBankManager::save()
{
    QJsonArray banksArray;
    
    for (const auto &bank : m_banks) {
        QJsonObject obj;
        obj["id"] = bank.id;
        obj["name"] = bank.name;
        obj["path"] = bank.path;
        obj["originalPath"] = bank.originalPath;
        obj["questionCount"] = bank.questionCount;
        obj["importTime"] = bank.importTime.toString(Qt::ISODate);
        obj["lastAccessTime"] = bank.lastAccessTime.toString(Qt::ISODate);
        obj["isAIParsed"] = bank.isAIParsed;
        obj["type"] = static_cast<int>(bank.type);
        
        banksArray.append(obj);
    }
    
    QJsonObject root;
    root["banks"] = banksArray;
    root["currentBankId"] = m_currentBankId;
    
    QJsonDocument doc(root);
    
    QString filePath = getConfigFilePath();
    QFile file(filePath);
    
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        file.close();
        qDebug() << "Question banks config saved to" << filePath;
    } else {
        qWarning() << "Failed to save question banks config";
    }
}

void QuestionBankManager::load()
{
    QString filePath = getConfigFilePath();
    QFile file(filePath);
    
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "No existing question banks config found";
        return;
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    
    if (!doc.isObject()) {
        qWarning() << "Invalid question banks config format";
        return;
    }
    
    QJsonObject root = doc.object();
    m_currentBankId = root["currentBankId"].toString();
    
    QJsonArray banksArray = root["banks"].toArray();
    m_banks.clear();
    
    for (const QJsonValue &val : banksArray) {
        QJsonObject obj = val.toObject();
        
        QuestionBankInfo info;
        info.id = obj["id"].toString();
        info.name = obj["name"].toString();
        info.path = obj["path"].toString();
        info.originalPath = obj["originalPath"].toString();
        info.questionCount = obj["questionCount"].toInt();
        info.importTime = QDateTime::fromString(obj["importTime"].toString(), Qt::ISODate);
        info.lastAccessTime = QDateTime::fromString(obj["lastAccessTime"].toString(), Qt::ISODate);
        info.isAIParsed = obj["isAIParsed"].toBool();
        info.type = static_cast<QuestionBankType>(obj["type"].toInt(1)); // 默认为 Processed
        
        m_banks.append(info);
    }
    
    qDebug() << "Loaded" << m_banks.size() << "question banks";
}
