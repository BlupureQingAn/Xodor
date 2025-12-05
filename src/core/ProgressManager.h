#ifndef PROGRESSMANAGER_H
#define PROGRESSMANAGER_H

#include <QObject>
#include <QMap>
#include "QuestionProgress.h"

// 进度管理器 - 单例模式
class ProgressManager : public QObject
{
    Q_OBJECT
public:
    static ProgressManager& instance();
    
    // 加载和保存进度
    void load();
    void save();
    
    // 获取进度记录
    QuestionProgressRecord getProgress(const QString &questionId) const;
    
    // 更新进度
    void recordAttempt(const QString &questionId, bool correct, const QString &code = QString());
    void updateStatus(const QString &questionId, QuestionStatus status);
    void saveLastCode(const QString &questionId, const QString &code);
    void setQuestionTitle(const QString &questionId, const QString &title);  // 设置题目标题
    
    // AI判定相关
    void recordAIJudge(const QString &questionId, bool passed, const QString &comment = QString());
    bool isAIJudgePassed(const QString &questionId) const;
    
    // 统计信息
    int getTotalQuestions() const { return m_progressMap.size(); }
    int getCompletedCount() const;
    int getMasteredCount() const;
    double getOverallAccuracy() const;
    
    // 刷题统计（新增）
    int getTotalCompleted() const;  // 总完成数（已完成+已掌握）
    int getCurrentStreak() const;   // 当前连续天数
    int getLongestStreak() const;   // 最长连续天数
    int getTodayCompleted() const;  // 今日完成数
    QMap<QDate, int> getActivityByDate(int days = 84) const;  // 获取最近N天的活动数据
    QMap<QString, int> getDifficultyDistribution() const;  // 难度分布（简单/中等/困难）
    
    // 按状态筛选
    QStringList getQuestionsByStatus(QuestionStatus status) const;
    
    // 清空进度
    void clear();
    void clearQuestion(const QString &questionId);
    
signals:
    void progressUpdated(const QString &questionId);
    void statisticsChanged();
    
private:
    ProgressManager(QObject *parent = nullptr);
    ~ProgressManager();
    ProgressManager(const ProgressManager&) = delete;
    ProgressManager& operator=(const ProgressManager&) = delete;
    
    QString getProgressFilePath() const;
    
    QMap<QString, QuestionProgressRecord> m_progressMap;
};

#endif // PROGRESSMANAGER_H
