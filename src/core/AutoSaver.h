#ifndef AUTOSAVER_H
#define AUTOSAVER_H

#include <QObject>
#include <QTimer>
#include <QString>

class CodeVersionManager;

class AutoSaver : public QObject
{
    Q_OBJECT
public:
    explicit AutoSaver(QObject *parent = nullptr);
    
    void setQuestionId(const QString &id);
    void setContent(const QString &content);
    void triggerSave();
    void forceSave();  // 立即保存，不使用定时器
    
    // 设置版本管理器（可选）
    void setVersionManager(CodeVersionManager *versionManager);
    
    // 保存版本（在测试通过时调用）
    void saveVersion(bool testPassed, int passedTests, int totalTests);
    
signals:
    void saved(const QString &questionId, const QString &content);
    void versionSaved(const QString &questionId);
    
private slots:
    void performSave();
    
private:
    QTimer *m_debounceTimer;
    QString m_questionId;
    QString m_content;
    CodeVersionManager *m_versionManager;
    static const int DEBOUNCE_MS = 200;  // 减少到200ms，更快响应
};

#endif // AUTOSAVER_H
