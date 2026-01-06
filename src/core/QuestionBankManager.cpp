#include "QuestionBankManager.h"
#include "../utils/ImportRuleManager.h"
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
    // 确保路径是相对于项目根目录的基础题库路径
    QString normalizedPath = sourcePath;
    
    // 如果传入的是绝对路径或其他路径，转换为基础题库路径
    if (!normalizedPath.startsWith("data/基础题库/")) {
        // 提取题库名称，构建正确的基础题库路径
        normalizedPath = QString("data/基础题库/%1").arg(name);
        qDebug() << "Normalized bank path from" << sourcePath << "to" << normalizedPath;
    }
    
    // 【关键修复】检查题库是否已经存在（防止重复注册）
    QString cleanedPath = QDir::cleanPath(normalizedPath);
    for (const QuestionBankInfo &existingBank : m_banks) {
        QString existingCleanedPath = QDir::cleanPath(existingBank.path);
        
        // 通过名称或路径判断是否已存在
        if (existingBank.name == name || 
            existingCleanedPath == cleanedPath ||
            existingCleanedPath.endsWith("/" + name) ||
            existingCleanedPath.endsWith("\\" + name)) {
            
            qDebug() << "[QuestionBankManager] Bank already exists:" << name 
                     << "ID:" << existingBank.id << "Path:" << existingBank.path;
            qDebug() << "[QuestionBankManager] Skipping duplicate registration";
            
            // 设置为当前题库
            m_currentBankId = existingBank.id;
            emit currentBankChanged(existingBank.id);
            
            return existingBank.id;  // 返回已存在的题库ID
        }
    }
    
    // 题库不存在，创建新的
    qDebug() << "[QuestionBankManager] Registering new bank:" << name;
    
    // 生成唯一ID
    QString bankId = generateBankId();
    
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
    
    qDebug() << "[QuestionBankManager] Question bank registered successfully:" << bankId << "at" << normalizedPath;
    
    return bankId;
}

bool QuestionBankManager::deleteQuestionBank(const QString &bankId)
{
    // 查找题库
    int index = -1;
    QString bankName;
    for (int i = 0; i < m_banks.size(); ++i) {
        if (m_banks[i].id == bankId) {
            index = i;
            bankName = m_banks[i].name;
            break;
        }
    }
    
    if (index < 0) {
        qWarning() << "Question bank not found:" << bankId;
        return false;
    }
    
    // 添加到忽略列表，防止自动重新注册
    addToIgnoreList(bankName);
    qDebug() << "[QuestionBankManager] Added to ignore list:" << bankName;
    
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
    
    qDebug() << "[QuestionBankManager] Question bank unregistered:" << bankId;
    
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
    
    // 保存忽略列表
    QJsonArray ignoredArray;
    for (const QString &bankName : m_ignoredBanks) {
        ignoredArray.append(bankName);
    }
    
    QJsonObject root;
    root["banks"] = banksArray;
    root["currentBankId"] = m_currentBankId;
    root["ignoredBanks"] = ignoredArray;
    
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
        qDebug() << "No existing question banks config found at" << filePath;
        // 配置文件不存在，尝试自动扫描
        qDebug() << "Attempting to auto-scan question banks...";
        int scanned = scanAndRegisterUnregisteredBanks();
        qDebug() << "Auto-scanned and registered" << scanned << "question banks";
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
    
    // 加载忽略列表
    m_ignoredBanks.clear();
    QJsonArray ignoredArray = root["ignoredBanks"].toArray();
    for (const QJsonValue &val : ignoredArray) {
        m_ignoredBanks.insert(val.toString());
    }
    qDebug() << "[QuestionBankManager] Loaded" << m_ignoredBanks.size() << "ignored banks:" << m_ignoredBanks;
    
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
    
    qDebug() << "Loaded" << m_banks.size() << "question banks from config";
    
    // 加载后自动验证和修复路径
    validateAndFixBankPaths();
    
    // 加载后也尝试扫描新的题库
    qDebug() << "Scanning for new question banks...";
    int newBanks = scanAndRegisterUnregisteredBanks();
    if (newBanks > 0) {
        qDebug() << "Found and registered" << newBanks << "new question banks";
    }
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


int QuestionBankManager::scanAndRegisterUnregisteredBanks()
{
    QString baseBankRoot = getProcessedBanksRoot();
    qDebug() << "[QuestionBankManager] Scanning base bank root:" << baseBankRoot;
    
    QDir baseDir(baseBankRoot);
    
    if (!baseDir.exists()) {
        qWarning() << "[QuestionBankManager] Base question bank directory does not exist:" << baseBankRoot;
        qDebug() << "[QuestionBankManager] Attempting to create directory...";
        if (baseDir.mkpath(".")) {
            qDebug() << "[QuestionBankManager] Directory created successfully";
        } else {
            qWarning() << "[QuestionBankManager] Failed to create directory";
        }
        return 0;
    }
    
    // 获取所有子目录（每个子目录代表一个题库）
    QStringList bankDirs = baseDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    qDebug() << "[QuestionBankManager] Found" << bankDirs.size() << "directories:" << bankDirs;
    qDebug() << "[QuestionBankManager] Currently registered banks:" << m_banks.size();
    
    int registeredCount = 0;
    
    for (const QString &bankName : bankDirs) {
        qDebug() << "[QuestionBankManager] Checking directory:" << bankName;
        
        // 检查是否在忽略列表中
        if (isInIgnoreList(bankName)) {
            qDebug() << "[QuestionBankManager]   In ignore list (user removed), skipping";
            continue;
        }
        
        // 检查是否已经注册（更严格的检查）
        bool alreadyRegistered = false;
        QString expectedPath = QString("%1/%2").arg(baseBankRoot).arg(bankName);
        for (const QuestionBankInfo &info : m_banks) {
            // 使用规范化路径比较，避免重复注册
            QString normalizedInfoPath = QDir::cleanPath(info.path);
            QString normalizedExpectedPath = QDir::cleanPath(expectedPath);
            
            if (info.name == bankName || 
                normalizedInfoPath == normalizedExpectedPath ||
                normalizedInfoPath.endsWith("/" + bankName) ||
                normalizedInfoPath.endsWith("\\" + bankName)) {
                alreadyRegistered = true;
                qDebug() << "[QuestionBankManager]   Already registered as:" << info.name << "at" << info.path;
                break;
            }
        }
        
        if (alreadyRegistered) {
            continue;
        }
        
        qDebug() << "[QuestionBankManager]   Not registered, checking for questions...";
        
        // 检查目录中是否有题目文件
        QString bankPath = QString("%1/%2").arg(baseBankRoot).arg(bankName);
        QDir bankDir(bankPath);
        
        // 递归统计所有.md文件（排除配置文件）
        QStringList filters;
        filters << "*.md";
        QFileInfoList mdFiles = bankDir.entryInfoList(filters, QDir::Files);
        
        // 也检查子目录（简单/中等/困难）
        QStringList subDirs = bankDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString &subDir : subDirs) {
            QDir subDirObj(bankPath + "/" + subDir);
            QFileInfoList subFiles = subDirObj.entryInfoList(filters, QDir::Files);
            
            // 过滤掉配置文件（虽然.md不太可能是配置文件，但保持一致性）
            for (const QFileInfo &fileInfo : subFiles) {
                if (!isConfigFile(fileInfo.fileName())) {
                    mdFiles.append(fileInfo);
                }
            }
        }
        
        qDebug() << "[QuestionBankManager]   Found" << mdFiles.size() << "question files";
        
        if (mdFiles.isEmpty()) {
            qDebug() << "[QuestionBankManager]   Skipping empty directory:" << bankName;
            continue;
        }
        
        // 注册这个题库
        qDebug() << "[QuestionBankManager]   Registering question bank:" << bankName << "with" << mdFiles.size() << "questions";
        
        QuestionBankInfo info;
        info.id = generateBankId();
        info.name = bankName;
        info.path = bankPath;
        info.originalPath = QString("data/原始题库/%1").arg(bankName);
        info.questionCount = mdFiles.size();
        info.importTime = QDateTime::currentDateTime();
        info.lastAccessTime = info.importTime;
        info.isAIParsed = true;  // 假设基础题库中的都是AI解析过的
        info.type = QuestionBankType::Processed;
        
        m_banks.append(info);
        registeredCount++;
        
        qDebug() << "[QuestionBankManager]   Successfully registered with ID:" << info.id;
    }
    
    if (registeredCount > 0) {
        qDebug() << "[QuestionBankManager] Saving configuration...";
        save();
        emit bankListChanged();
        qDebug() << "[QuestionBankManager] ✓ Auto-registered" << registeredCount << "new question banks";
    } else {
        qDebug() << "[QuestionBankManager] No new question banks to register";
    }
    
    return registeredCount;
}


bool QuestionBankManager::deleteQuestionBankCompletely(const QString &bankId)
{
    qDebug() << "[QuestionBankManager] deleteQuestionBankCompletely:" << bankId;
    
    // 获取题库信息
    QuestionBankInfo info = getBankInfo(bankId);
    if (info.id.isEmpty()) {
        qWarning() << "[QuestionBankManager] Bank not found:" << bankId;
        return false;
    }
    
    QString bankName = info.name;
    qDebug() << "[QuestionBankManager] Deleting bank:" << bankName;
    qDebug() << "[QuestionBankManager] Path:" << info.path;
    qDebug() << "[QuestionBankManager] Original path:" << info.originalPath;
    
    // 先从配置中移除（会添加到忽略列表）
    if (!deleteQuestionBank(bankId)) {
        qWarning() << "[QuestionBankManager] Failed to remove from config";
        return false;
    }
    
    bool success = true;
    
    // 删除基础题库文件夹
    QDir processedDir(info.path);
    if (processedDir.exists()) {
        qDebug() << "[QuestionBankManager] Deleting processed bank directory:" << info.path;
        if (processedDir.removeRecursively()) {
            qDebug() << "[QuestionBankManager] ✓ Processed bank directory deleted";
        } else {
            qWarning() << "[QuestionBankManager] ✗ Failed to delete processed bank directory";
            success = false;
        }
    } else {
        qDebug() << "[QuestionBankManager] Processed bank directory does not exist";
    }
    
    // 删除原始题库文件夹（如果存在）
    QDir originalDir(info.originalPath);
    if (originalDir.exists()) {
        qDebug() << "[QuestionBankManager] Deleting original bank directory:" << info.originalPath;
        if (originalDir.removeRecursively()) {
            qDebug() << "[QuestionBankManager] ✓ Original bank directory deleted";
        } else {
            qWarning() << "[QuestionBankManager] ✗ Failed to delete original bank directory";
            success = false;
        }
    } else {
        qDebug() << "[QuestionBankManager] Original bank directory does not exist";
    }
    
    // 删除导入规则文件（如果存在）
    if (ImportRuleManager::hasImportRule(bankName)) {
        qDebug() << "[QuestionBankManager] Deleting import rule file:" << ImportRuleManager::getRulePath(bankName);
        if (ImportRuleManager::deleteImportRule(bankName)) {
            qDebug() << "[QuestionBankManager] ✓ Import rule file deleted";
        } else {
            qWarning() << "[QuestionBankManager] ✗ Failed to delete import rule file";
            success = false;
        }
    } else {
        qDebug() << "[QuestionBankManager] No import rule file found for this bank";
    }
    
    // 完全删除后，从忽略列表中移除（因为文件已经不存在了）
    removeFromIgnoreList(bankName);
    qDebug() << "[QuestionBankManager] Removed from ignore list:" << bankName;
    
    if (success) {
        qDebug() << "[QuestionBankManager] ✓ Bank completely deleted:" << bankName;
    } else {
        qWarning() << "[QuestionBankManager] ⚠ Bank partially deleted (some files may remain)";
    }
    
    return success;
}


// ==================== 忽略列表管理 ====================

void QuestionBankManager::addToIgnoreList(const QString &bankName)
{
    m_ignoredBanks.insert(bankName);
    save();  // 立即保存
}

void QuestionBankManager::removeFromIgnoreList(const QString &bankName)
{
    m_ignoredBanks.remove(bankName);
    save();  // 立即保存
}

bool QuestionBankManager::isInIgnoreList(const QString &bankName) const
{
    return m_ignoredBanks.contains(bankName);
}

void QuestionBankManager::clearIgnoreList()
{
    m_ignoredBanks.clear();
    save();  // 立即保存
}

// ==================== 配置文件过滤 ====================

bool QuestionBankManager::isConfigFile(const QString &fileName) const
{
    // 过滤导入规则文件和其他配置文件
    // 1. 导入规则文件：*_parse_rule.json
    // 2. 其他可能的配置文件：*.json（在题库目录中的JSON文件通常是配置）
    
    if (fileName.endsWith("_parse_rule.json")) {
        return true;
    }
    
    // 可选：过滤所有JSON文件（如果题库目录中不应该有JSON文件）
    // 注意：有些题库可能使用questions.json存储题目，需要根据实际情况调整
    // if (fileName.endsWith(".json")) {
    //     return true;
    // }
    
    return false;
}
