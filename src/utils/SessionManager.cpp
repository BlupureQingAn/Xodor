#include "SessionManager.h"
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDateTime>

SessionManager& SessionManager::instance()
{
    static SessionManager inst;
    return inst;
}

QString SessionManager::sessionFilePath() const
{
    return "data/last_session.json";
}

QString SessionManager::codeBackupPath(const QString &questionId) const
{
    return QString("data/code_backup/%1.json").arg(questionId);
}

QString SessionManager::backupDirPath() const
{
    return "data/session_backups";
}

// SessionState 序列化
QJsonObject SessionState::toJson() const
{
    QJsonObject json;
    
    // 题库信息
    json["questionBankPath"] = questionBankPath;
    json["currentQuestionIndex"] = currentQuestionIndex;
    json["currentQuestionId"] = currentQuestionId;
    
    // 题库面板状态
    QJsonArray expandedArray;
    for (const QString &path : expandedBankPaths) {
        expandedArray.append(path);
    }
    json["expandedBankPaths"] = expandedArray;
    json["selectedQuestionPath"] = selectedQuestionPath;
    
    // 窗口状态
    json["windowGeometry"] = QString(windowGeometry.toBase64());
    json["windowState"] = QString(windowState.toBase64());
    
    // 代码编辑器状态
    json["currentCode"] = currentCode;
    json["currentLanguage"] = currentLanguage;
    json["cursorPosition"] = cursorPosition;
    
    // 刷题模式状态
    json["isPracticeMode"] = isPracticeMode;
    json["practiceCategory"] = practiceCategory;
    json["practiceQuestionIndex"] = practiceQuestionIndex;
    
    // 套题会话状态
    json["hasActiveExamSession"] = hasActiveExamSession;
    json["examSessionId"] = examSessionId;
    json["examName"] = examName;
    json["examStartTime"] = examStartTime.toString(Qt::ISODate);
    json["examTimeSpent"] = examTimeSpent;
    
    // AI助手状态
    json["aiAssistantVisible"] = aiAssistantVisible;
    
    // 最后保存时间
    json["lastSaved"] = lastSaved.toString(Qt::ISODate);
    
    return json;
}

SessionState SessionState::fromJson(const QJsonObject &json)
{
    SessionState state;
    
    // 题库信息
    state.questionBankPath = json["questionBankPath"].toString();
    state.currentQuestionIndex = json["currentQuestionIndex"].toInt();
    state.currentQuestionId = json["currentQuestionId"].toString();
    
    // 题库面板状态
    if (json.contains("expandedBankPaths")) {
        QJsonArray expandedArray = json["expandedBankPaths"].toArray();
        for (const QJsonValue &value : expandedArray) {
            state.expandedBankPaths.append(value.toString());
        }
    }
    state.selectedQuestionPath = json["selectedQuestionPath"].toString();
    
    // 窗口状态
    if (json.contains("windowGeometry")) {
        state.windowGeometry = QByteArray::fromBase64(
            json["windowGeometry"].toString().toUtf8());
    }
    if (json.contains("windowState")) {
        state.windowState = QByteArray::fromBase64(
            json["windowState"].toString().toUtf8());
    }
    
    // 代码编辑器状态
    state.currentCode = json["currentCode"].toString();
    state.currentLanguage = json["currentLanguage"].toString();
    state.cursorPosition = json["cursorPosition"].toInt();
    
    // 刷题模式状态
    state.isPracticeMode = json["isPracticeMode"].toBool();
    state.practiceCategory = json["practiceCategory"].toString();
    state.practiceQuestionIndex = json["practiceQuestionIndex"].toInt();
    
    // 套题会话状态
    state.hasActiveExamSession = json["hasActiveExamSession"].toBool();
    state.examSessionId = json["examSessionId"].toString();
    state.examName = json["examName"].toString();
    state.examStartTime = QDateTime::fromString(
        json["examStartTime"].toString(), Qt::ISODate);
    state.examTimeSpent = json["examTimeSpent"].toInt();
    
    // AI助手状态
    state.aiAssistantVisible = json["aiAssistantVisible"].toBool();
    
    // 最后保存时间
    state.lastSaved = QDateTime::fromString(
        json["lastSaved"].toString(), Qt::ISODate);
    
    return state;
}

// 基本会话管理
void SessionManager::saveSession(const QString &questionBankPath, int currentQuestionIndex, const QString &questionId)
{
    // 加载现有的 SessionState 并更新
    SessionState state = loadSessionState();
    state.questionBankPath = questionBankPath;
    state.currentQuestionIndex = currentQuestionIndex;
    state.currentQuestionId = questionId;
    state.lastSaved = QDateTime::currentDateTime();
    
    // 保存完整的 SessionState
    saveSessionState(state);
}

bool SessionManager::loadSession(QString &questionBankPath, int &currentQuestionIndex)
{
    QString questionId;  // 忽略题目ID
    return loadSession(questionBankPath, currentQuestionIndex, questionId);
}

bool SessionManager::loadSession(QString &questionBankPath, int &currentQuestionIndex, QString &questionId)
{
    // 从完整的 SessionState 加载
    SessionState state = loadSessionState();
    questionBankPath = state.questionBankPath;
    currentQuestionIndex = state.currentQuestionIndex;
    questionId = state.currentQuestionId;
    
    return !questionBankPath.isEmpty();
}

// 完整会话状态管理
void SessionManager::saveSessionState(const SessionState &state)
{
    QDir dir("data");
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    
    QFile file(sessionFilePath());
    if (file.open(QIODevice::WriteOnly)) {
        QJsonDocument doc(state.toJson());
        file.write(doc.toJson(QJsonDocument::Indented));
        file.close();
    }
}

SessionState SessionManager::loadSessionState()
{
    QFile file(sessionFilePath());
    if (!file.open(QIODevice::ReadOnly)) {
        return SessionState();
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    
    if (!doc.isObject()) {
        return SessionState();
    }
    
    return SessionState::fromJson(doc.object());
}

bool SessionManager::hasValidSession()
{
    QFile file(sessionFilePath());
    if (!file.exists()) {
        return false;
    }
    
    SessionState state = loadSessionState();
    
    // 检查会话是否在最近7天内
    if (state.lastSaved.isValid()) {
        qint64 daysSinceLastSave = state.lastSaved.daysTo(QDateTime::currentDateTime());
        return daysSinceLastSave <= 7;
    }
    
    return false;
}

// 窗口状态
void SessionManager::saveWindowState(const QByteArray &geometry, const QByteArray &state)
{
    SessionState sessionState = loadSessionState();
    sessionState.windowGeometry = geometry;
    sessionState.windowState = state;
    sessionState.lastSaved = QDateTime::currentDateTime();
    saveSessionState(sessionState);
}

bool SessionManager::loadWindowState(QByteArray &geometry, QByteArray &state)
{
    SessionState sessionState = loadSessionState();
    geometry = sessionState.windowGeometry;
    state = sessionState.windowState;
    return !geometry.isEmpty();
}

// 代码自动保存
void SessionManager::saveCurrentCode(const QString &questionId, const QString &code,
                                    const QString &language, int cursorPosition)
{
    QDir dir("data/code_backup");
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    
    QJsonObject json;
    json["questionId"] = questionId;
    json["code"] = code;
    json["language"] = language;
    json["cursorPosition"] = cursorPosition;
    json["savedTime"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    
    QFile file(codeBackupPath(questionId));
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(json).toJson());
        file.close();
    }
    
    // 同时更新会话状态
    SessionState state = loadSessionState();
    state.currentCode = code;
    state.currentLanguage = language;
    state.cursorPosition = cursorPosition;
    state.lastSaved = QDateTime::currentDateTime();
    saveSessionState(state);
}

bool SessionManager::loadCurrentCode(const QString &questionId, QString &code,
                                     QString &language, int &cursorPosition)
{
    QFile file(codeBackupPath(questionId));
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    
    if (!doc.isObject()) {
        return false;
    }
    
    QJsonObject json = doc.object();
    code = json["code"].toString();
    language = json["language"].toString();
    cursorPosition = json["cursorPosition"].toInt();
    
    return !code.isEmpty();
}

// 套题会话管理
void SessionManager::saveExamSession(const QString &sessionId, const QString &examName,
                                    const QDateTime &startTime, int timeSpent)
{
    SessionState state = loadSessionState();
    state.hasActiveExamSession = true;
    state.examSessionId = sessionId;
    state.examName = examName;
    state.examStartTime = startTime;
    state.examTimeSpent = timeSpent;
    state.lastSaved = QDateTime::currentDateTime();
    saveSessionState(state);
}

bool SessionManager::loadExamSession(QString &sessionId, QString &examName,
                                    QDateTime &startTime, int &timeSpent)
{
    SessionState state = loadSessionState();
    
    if (!state.hasActiveExamSession) {
        return false;
    }
    
    sessionId = state.examSessionId;
    examName = state.examName;
    startTime = state.examStartTime;
    timeSpent = state.examTimeSpent;
    
    return true;
}

void SessionManager::clearExamSession()
{
    SessionState state = loadSessionState();
    state.hasActiveExamSession = false;
    state.examSessionId.clear();
    state.examName.clear();
    state.examStartTime = QDateTime();
    state.examTimeSpent = 0;
    state.lastSaved = QDateTime::currentDateTime();
    saveSessionState(state);
}

// 刷题模式状态
void SessionManager::savePracticeModeState(const QString &category, int questionIndex)
{
    SessionState state = loadSessionState();
    state.isPracticeMode = true;
    state.practiceCategory = category;
    state.practiceQuestionIndex = questionIndex;
    state.lastSaved = QDateTime::currentDateTime();
    saveSessionState(state);
}

bool SessionManager::loadPracticeModeState(QString &category, int &questionIndex)
{
    SessionState state = loadSessionState();
    
    if (!state.isPracticeMode) {
        return false;
    }
    
    category = state.practiceCategory;
    questionIndex = state.practiceQuestionIndex;
    
    return true;
}

// 清理
void SessionManager::clearSession()
{
    QFile::remove(sessionFilePath());
}

void SessionManager::clearOldSessions(int daysToKeep)
{
    QDir backupDir(backupDirPath());
    if (!backupDir.exists()) {
        return;
    }
    
    QDateTime cutoffDate = QDateTime::currentDateTime().addDays(-daysToKeep);
    
    QStringList backups = backupDir.entryList(QStringList() << "*.json", QDir::Files);
    for (const QString &backup : backups) {
        QString filePath = backupDir.filePath(backup);
        QFileInfo fileInfo(filePath);
        
        if (fileInfo.lastModified() < cutoffDate) {
            QFile::remove(filePath);
        }
    }
}

// 备份与恢复
bool SessionManager::createBackup(const QString &backupName)
{
    QDir backupDir(backupDirPath());
    if (!backupDir.exists()) {
        backupDir.mkpath(".");
    }
    
    QString name = backupName.isEmpty() 
        ? QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss")
        : backupName;
    
    QString backupPath = backupDir.filePath(QString("session_%1.json").arg(name));
    
    return QFile::copy(sessionFilePath(), backupPath);
}

bool SessionManager::restoreFromBackup(const QString &backupName)
{
    QDir backupDir(backupDirPath());
    QString backupPath = backupDir.filePath(QString("session_%1.json").arg(backupName));
    
    if (!QFile::exists(backupPath)) {
        return false;
    }
    
    // 备份当前会话
    createBackup("before_restore");
    
    // 恢复
    QFile::remove(sessionFilePath());
    return QFile::copy(backupPath, sessionFilePath());
}

QStringList SessionManager::listBackups()
{
    QDir backupDir(backupDirPath());
    if (!backupDir.exists()) {
        return QStringList();
    }
    
    QStringList backups = backupDir.entryList(QStringList() << "session_*.json", 
                                             QDir::Files, QDir::Time);
    
    // 移除前缀和后缀
    for (QString &backup : backups) {
        backup.remove("session_");
        backup.remove(".json");
    }
    
    return backups;
}

// 题库面板状态管理
void SessionManager::savePanelState(const QStringList &expandedPaths, const QString &selectedQuestionPath)
{
    // 加载现有的 SessionState 并更新面板状态
    SessionState state = loadSessionState();
    state.expandedBankPaths = expandedPaths;
    state.selectedQuestionPath = selectedQuestionPath;
    state.lastSaved = QDateTime::currentDateTime();
    
    // 保存完整的 SessionState
    saveSessionState(state);
}

bool SessionManager::loadPanelState(QStringList &expandedPaths, QString &selectedQuestionPath)
{
    SessionState state = loadSessionState();
    expandedPaths = state.expandedBankPaths;
    selectedQuestionPath = state.selectedQuestionPath;
    return !expandedPaths.isEmpty() || !selectedQuestionPath.isEmpty();
}
