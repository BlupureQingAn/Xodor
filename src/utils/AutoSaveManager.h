#ifndef AUTOSAVEMANAGER_H
#define AUTOSAVEMANAGER_H

#include <QObject>
#include <QTimer>
#include <QString>

// 自动保存管理器
class AutoSaveManager : public QObject
{
    Q_OBJECT
public:
    static AutoSaveManager& instance();
    
    // 启动/停止自动保存
    void start(int intervalSeconds = 180);  // 默认3分钟
    void stop();
    bool isRunning() const { return m_timer->isActive(); }
    
    // 设置保存间隔
    void setInterval(int seconds);
    int interval() const { return m_intervalSeconds; }
    
    // 标记需要保存
    void markDirty();
    void markClean();
    bool isDirty() const { return m_isDirty; }
    
    // 立即保存
    void saveNow();
    
signals:
    void autoSaveTriggered();
    void saveCompleted();
    void saveFailed(const QString &error);
    
private slots:
    void onTimerTimeout();
    
private:
    AutoSaveManager(QObject *parent = nullptr);
    ~AutoSaveManager();
    AutoSaveManager(const AutoSaveManager&) = delete;
    AutoSaveManager& operator=(const AutoSaveManager&) = delete;
    
    QTimer *m_timer;
    int m_intervalSeconds;
    bool m_isDirty;
};

#endif // AUTOSAVEMANAGER_H
