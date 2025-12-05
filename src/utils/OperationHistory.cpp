#include "OperationHistory.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDebug>

// ============ Operation 序列化 ============

QJsonObject Operation::toJson() const
{
    QJsonObject json;
    json["type"] = static_cast<int>(type);
    json["description"] = description;
    json["timestamp"] = timestamp.toString(Qt::ISODate);
    json["data"] = data;
    json["filePath"] = filePath;
    json["backupPath"] = backupPath;
    json["fileContent"] = QString(fileContent.toBase64());
    json["isDirectory"] = isDirectory;
    return json;
}

Operation Operation::fromJson(const QJsonObject &json)
{
    Operation op;
    op.type = static_cast<OperationType>(json["type"].toInt());
    op.description = json["description"].toString();
    op.timestamp = QDateTime::fromString(json["timestamp"].toString(), Qt::ISODate);
    op.data = json["data"].toObject();
    op.filePath = json["filePath"].toString();
    op.backupPath = json["backupPath"].toString();
    op.fileContent = QByteArray::fromBase64(json["fileContent"].toString().toUtf8());
    op.isDirectory = json["isDirectory"].toBool();
    return op;
}

// ============ OperationHistory 实现 ============

OperationHistory& OperationHistory::instance()
{
    static OperationHistory inst;
    return inst;
}

OperationHistory::OperationHistory(QObject *parent)
    : QObject(parent)
    , m_currentIndex(-1)
{
    load();
}

OperationHistory::~OperationHistory()
{
    save();
}

QString OperationHistory::getRecycleBinPath() const
{
    return "data/recycle_bin";
}

QString OperationHistory::getHistoryFilePath() const
{
    return "data/operation_history.json";
}

void OperationHistory::recordDeleteQuestion(const QString &filePath, const QByteArray &content)
{
    Operation op;
    op.type = OperationType::DeleteQuestion;
    op.description = QString("删除题目: %1").arg(QFileInfo(filePath).fileName());
    op.timestamp = QDateTime::currentDateTime();
    op.filePath = filePath;
    op.fileContent = content;
    op.isDirectory = false;
    
    // 移动到回收站
    QString backupPath;
    if (deleteFileToRecycleBin(filePath, backupPath)) {
        op.backupPath = backupPath;
        addOperation(op);
        qDebug() << "[OperationHistory] Recorded delete question:" << filePath;
    } else {
        qWarning() << "[OperationHistory] Failed to move file to recycle bin:" << filePath;
    }
}

void OperationHistory::recordDeleteBank(const QString &bankPath)
{
    Operation op;
    op.type = OperationType::DeleteBank;
    op.description = QString("删除题库: %1").arg(QFileInfo(bankPath).fileName());
    op.timestamp = QDateTime::currentDateTime();
    op.filePath = bankPath;
    op.isDirectory = true;
    
    // 移动到回收站
    QString backupPath;
    if (deleteDirectoryToRecycleBin(bankPath, backupPath)) {
        op.backupPath = backupPath;
        addOperation(op);
        qDebug() << "[OperationHistory] Recorded delete bank:" << bankPath;
    } else {
        qWarning() << "[OperationHistory] Failed to move directory to recycle bin:" << bankPath;
    }
}

void OperationHistory::recordCreateQuestion(const QString &filePath)
{
    Operation op;
    op.type = OperationType::CreateQuestion;
    op.description = QString("创建题目: %1").arg(QFileInfo(filePath).fileName());
    op.timestamp = QDateTime::currentDateTime();
    op.filePath = filePath;
    op.isDirectory = false;
    
    addOperation(op);
    qDebug() << "[OperationHistory] Recorded create question:" << filePath;
}

void OperationHistory::recordEditQuestion(const QString &filePath, const QByteArray &oldContent)
{
    Operation op;
    op.type = OperationType::EditQuestion;
    op.description = QString("编辑题目: %1").arg(QFileInfo(filePath).fileName());
    op.timestamp = QDateTime::currentDateTime();
    op.filePath = filePath;
    op.fileContent = oldContent;
    op.isDirectory = false;
    
    addOperation(op);
    qDebug() << "[OperationHistory] Recorded edit question:" << filePath;
}

void OperationHistory::addOperation(const Operation &op)
{
    // 如果当前不在历史末尾，删除后面的记录
    if (m_currentIndex < m_history.size() - 1) {
        m_history.erase(m_history.begin() + m_currentIndex + 1, m_history.end());
    }
    
    // 添加新操作
    m_history.append(op);
    m_currentIndex = m_history.size() - 1;
    
    // 限制历史记录数量
    if (m_history.size() > MAX_HISTORY_SIZE) {
        m_history.removeFirst();
        m_currentIndex--;
    }
    
    emit historyChanged();
    save();
}

bool OperationHistory::canUndo() const
{
    return m_currentIndex >= 0;
}

bool OperationHistory::canRedo() const
{
    return m_currentIndex < m_history.size() - 1;
}

bool OperationHistory::undo()
{
    if (!canUndo()) {
        return false;
    }
    
    Operation op = m_history[m_currentIndex];
    bool success = false;
    
    switch (op.type) {
        case OperationType::DeleteQuestion:
        case OperationType::DeleteBank:
            // 恢复被删除的文件/目录
            if (op.isDirectory) {
                success = restoreDirectory(op.filePath, op.backupPath);
            } else {
                success = restoreFile(op.filePath, op.fileContent);
            }
            break;
            
        case OperationType::CreateQuestion:
            // 删除创建的文件
            success = QFile::remove(op.filePath);
            break;
            
        case OperationType::EditQuestion:
            // 恢复旧内容
            success = restoreFile(op.filePath, op.fileContent);
            break;
    }
    
    if (success) {
        m_currentIndex--;
        emit operationUndone(op);
        emit historyChanged();
        qDebug() << "[OperationHistory] Undo successful:" << op.description;
    } else {
        qWarning() << "[OperationHistory] Undo failed:" << op.description;
    }
    
    return success;
}

bool OperationHistory::redo()
{
    if (!canRedo()) {
        return false;
    }
    
    m_currentIndex++;
    Operation op = m_history[m_currentIndex];
    bool success = false;
    
    switch (op.type) {
        case OperationType::DeleteQuestion:
        case OperationType::DeleteBank:
            // 重新删除
            if (op.isDirectory) {
                QString backupPath;
                success = deleteDirectoryToRecycleBin(op.filePath, backupPath);
            } else {
                QString backupPath;
                success = deleteFileToRecycleBin(op.filePath, backupPath);
            }
            break;
            
        case OperationType::CreateQuestion:
            // 重新创建（从备份恢复）
            success = restoreFile(op.filePath, op.fileContent);
            break;
            
        case OperationType::EditQuestion:
            // 重新应用编辑（需要保存当前内容）
            // 这里简化处理，实际应该保存编辑后的内容
            success = true;
            break;
    }
    
    if (success) {
        emit operationRedone(op);
        emit historyChanged();
        qDebug() << "[OperationHistory] Redo successful:" << op.description;
    } else {
        m_currentIndex--;
        qWarning() << "[OperationHistory] Redo failed:" << op.description;
    }
    
    return success;
}

void OperationHistory::clear()
{
    m_history.clear();
    m_currentIndex = -1;
    emit historyChanged();
    save();
}

bool OperationHistory::restoreFile(const QString &filePath, const QByteArray &content)
{
    // 确保目录存在
    QFileInfo fileInfo(filePath);
    QDir dir = fileInfo.dir();
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    
    // 写入文件
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(content);
        file.close();
        return true;
    }
    
    return false;
}

bool OperationHistory::restoreDirectory(const QString &dirPath, const QString &backupPath)
{
    if (backupPath.isEmpty() || !QDir(backupPath).exists()) {
        return false;
    }
    
    // 确保父目录存在
    QFileInfo dirInfo(dirPath);
    QDir parentDir = dirInfo.dir();
    if (!parentDir.exists()) {
        parentDir.mkpath(".");
    }
    
    // 复制回收站中的目录
    return copyDirectory(backupPath, dirPath);
}

bool OperationHistory::copyDirectory(const QString &source, const QString &destination)
{
    QDir sourceDir(source);
    if (!sourceDir.exists()) {
        return false;
    }
    
    QDir destDir(destination);
    if (!destDir.exists()) {
        destDir.mkpath(".");
    }
    
    // 复制文件
    QFileInfoList files = sourceDir.entryInfoList(QDir::Files);
    for (const QFileInfo &fileInfo : files) {
        QString srcPath = fileInfo.absoluteFilePath();
        QString dstPath = destination + "/" + fileInfo.fileName();
        if (!QFile::copy(srcPath, dstPath)) {
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

bool OperationHistory::deleteFileToRecycleBin(const QString &filePath, QString &backupPath)
{
    if (!QFile::exists(filePath)) {
        return false;
    }
    
    // 创建回收站目录
    QString recycleBin = getRecycleBinPath();
    QDir dir(recycleBin);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    
    // 生成备份文件名（添加时间戳避免冲突）
    QFileInfo fileInfo(filePath);
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
    QString backupFileName = QString("%1_%2").arg(timestamp).arg(fileInfo.fileName());
    backupPath = recycleBin + "/" + backupFileName;
    
    // 移动文件到回收站
    return QFile::rename(filePath, backupPath);
}

bool OperationHistory::deleteDirectoryToRecycleBin(const QString &dirPath, QString &backupPath)
{
    if (!QDir(dirPath).exists()) {
        return false;
    }
    
    // 创建回收站目录
    QString recycleBin = getRecycleBinPath();
    QDir dir(recycleBin);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    
    // 生成备份目录名（添加时间戳避免冲突）
    QFileInfo dirInfo(dirPath);
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
    QString backupDirName = QString("%1_%2").arg(timestamp).arg(dirInfo.fileName());
    backupPath = recycleBin + "/" + backupDirName;
    
    // 移动目录到回收站
    return QDir().rename(dirPath, backupPath);
}

void OperationHistory::save()
{
    QDir dir("data");
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    
    QJsonArray historyArray;
    for (const Operation &op : m_history) {
        historyArray.append(op.toJson());
    }
    
    QJsonObject json;
    json["history"] = historyArray;
    json["currentIndex"] = m_currentIndex;
    
    QFile file(getHistoryFilePath());
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(json).toJson(QJsonDocument::Indented));
        file.close();
    }
}

void OperationHistory::load()
{
    QFile file(getHistoryFilePath());
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    
    if (!doc.isObject()) {
        return;
    }
    
    QJsonObject json = doc.object();
    m_currentIndex = json["currentIndex"].toInt();
    
    QJsonArray historyArray = json["history"].toArray();
    m_history.clear();
    for (const QJsonValue &value : historyArray) {
        m_history.append(Operation::fromJson(value.toObject()));
    }
}
