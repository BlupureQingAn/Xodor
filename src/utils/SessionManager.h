#ifndef SESSIONMANAGER_H
#define SESSIONMANAGER_H

#include <QString>
#include <QJsonObject>
#include <QDateTime>

// 会话状态
struct SessionState {
    // 题库信息
    QString questionBankPath;
    int currentQuestionIndex;
    QString currentQuestionId;  // 当前题目ID
    
    // 题库面板状态
    QStringList expandedBankPaths;  // 展开的题库路径列表
    QString selectedQuestionPath;   // 选中的题目文件路径
    
    // 窗口状态
    QByteArray windowGeometry;
    QByteArray windowState;
    
    // 代码编辑器状态
    QString currentCode;
    QString currentLanguage;
    int cursorPosition;
    
    // 刷题模式状态
    bool isPracticeMode;
    QString practiceCategory;
    int practiceQuestionIndex;
    
    // 套题会话状态
    bool hasActiveExamSession;
    QString examSessionId;
    QString examName;
    QDateTime examStartTime;
    int examTimeSpent;  // 已用时间（秒）
    
    // AI助手状态
    bool aiAssistantVisible;
    
    // 最后保存时间
    QDateTime lastSaved;
    
    QJsonObject toJson() const;
    static SessionState fromJson(const QJsonObject &json);
};

class SessionManager
{
public:
    static SessionManager& instance();
    
    // 基本会话管理
    void saveSession(const QString &questionBankPath, int currentQuestionIndex, const QString &questionId = QString());
    bool loadSession(QString &questionBankPath, int &currentQuestionIndex);
    bool loadSession(QString &questionBankPath, int &currentQuestionIndex, QString &questionId);
    
    // 题库面板状态管理
    void savePanelState(const QStringList &expandedPaths, const QString &selectedQuestionPath);
    bool loadPanelState(QStringList &expandedPaths, QString &selectedQuestionPath);
    
    // 完整会话状态管理
    void saveSessionState(const SessionState &state);
    SessionState loadSessionState();
    bool hasValidSession();
    
    // 窗口状态
    void saveWindowState(const QByteArray &geometry, const QByteArray &state);
    bool loadWindowState(QByteArray &geometry, QByteArray &state);
    
    // 代码自动保存
    void saveCurrentCode(const QString &questionId, const QString &code, 
                        const QString &language, int cursorPosition);
    bool loadCurrentCode(const QString &questionId, QString &code, 
                        QString &language, int &cursorPosition);
    
    // 套题会话管理
    void saveExamSession(const QString &sessionId, const QString &examName,
                        const QDateTime &startTime, int timeSpent);
    bool loadExamSession(QString &sessionId, QString &examName,
                        QDateTime &startTime, int &timeSpent);
    void clearExamSession();
    
    // 刷题模式状态
    void savePracticeModeState(const QString &category, int questionIndex);
    bool loadPracticeModeState(QString &category, int &questionIndex);
    
    // 清理
    void clearSession();
    void clearOldSessions(int daysToKeep = 7);
    
    // 备份与恢复
    bool createBackup(const QString &backupName = QString());
    bool restoreFromBackup(const QString &backupName);
    QStringList listBackups();
    
private:
    SessionManager() = default;
    SessionManager(const SessionManager&) = delete;
    SessionManager& operator=(const SessionManager&) = delete;
    
    QString sessionFilePath() const;
    QString codeBackupPath(const QString &questionId) const;
    QString backupDirPath() const;
};

#endif // SESSIONMANAGER_H
