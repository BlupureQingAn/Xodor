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
    
    // 确保路径是相对于项目根目录的基础题库路径
    QString normalizedPath = sourcePath;
    
    // 如果传入的是绝对路径或其他路径，转换为基础题库路径
    if (!normalizedPath.startsWith("data/基础题库/")) {
        // 提取题库名称，构建正确的基础题库路径
        normalizedPath = QString("data/基础题库/%1").arg(name);
        qDebug() << "Normalized bank path from" << sourcePath << "to" << normalizedPath;
    }
    
    // 创建题库信息
    QuestionBankInfo info;
    info.id = bankId;
    info.name = name;
    info.path = normalizedPath;  // 使用规范化的基础题库路径
    info.originalPath = QString("data/原始题库/%1").arg(name);  // 原始题库路径
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
    
    qDebug() << "Question bank registered successfully:" << bankId << "at" << normalizedPath;
    
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
    
    // 只从列表中移除，不删除实际文件
    // （文件在 data/基础题库/ 下，用户可能还需要）
    m_banks.removeAt(index);
    
    // 如果删除的是当前题库，清空当前题库
    if (m_currentBankId == bankId) {
        m_currentBankId.clear();
        emit currentBankChanged(QString());
    }
    
    // 保存配置
    save();
    
    emit bankListChanged();
    
    qDebug() << "Question bank unregistered:" << bankId;
    
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

bool QuestionBankManager::updateQuestionCount(const QString &bankId, int count)
{
    for (auto &bank : m_banks) {
        if (bank.id == bankId) {
            bank.questionCount = count;
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
    
    // 加载后自动验证和修复路径
    validateAndFixBankPaths();
}

bool QuestionBankManager::validateAndFixBankPaths()
{
    bool hasChanges = false;
    
    for (auto &bank : m_banks) {
        bool needsFix = false;
        QString oldPath = bank.path;
        
        // 检查路径是否正确
        // 1. 不应该包含 AppData 路径
        if (bank.path.contains("AppData") || bank.path.contains("Roaming")) {
            needsFix = true;
            qWarning() << "Found invalid AppData path for bank:" << bank.name;
        }
        
        // 2. 应该以 data/基础题库/ 开头
        if (!bank.path.startsWith("data/基础题库/") && !bank.path.startsWith("data\\基础题库\\")) {
            needsFix = true;
            qWarning() << "Found non-standard path for bank:" << bank.name;
        }
        
        // 3. 检查路径是否存在
        QDir dir(bank.path);
        if (!dir.exists()) {
            // 尝试在基础题库中查找
            QString expectedPath = QString("data/基础题库/%1").arg(bank.name);
            QDir expectedDir(expectedPath);
            if (expectedDir.exists()) {
                needsFix = true;
                qInfo() << "Found bank at expected location:" << expectedPath;
            } else {
                qWarning() << "Bank path does not exist:" << bank.path;
                qWarning() << "Expected path also does not exist:" << expectedPath;
            }
        }
        
        // 修复路径
        if (needsFix) {
            QString newPath = QString("data/基础题库/%1").arg(bank.name);
            QString newOriginalPath = QString("data/原始题库/%1").arg(bank.name);
            
            qInfo() << "Fixing bank path:";
            qInfo() << "  Bank:" << bank.name;
            qInfo() << "  Old path:" << oldPath;
            qInfo() << "  New path:" << newPath;
            
            bank.path = newPath;
            bank.originalPath = newOriginalPath;
            bank.type = QuestionBankType::Processed;
            
            hasChanges = true;
        }
    }
    
    // 如果有修改，保存配置
    if (hasChanges) {
        qInfo() << "Bank paths have been fixed, saving configuration...";
        save();
        emit bankListChanged();
    }
    
    return hasChanges;
}
