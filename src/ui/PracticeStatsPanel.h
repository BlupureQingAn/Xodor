#ifndef PRACTICESTATSPANEL_H
#define PRACTICESTATSPANEL_H

#include <QWidget>
#include <QLabel>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QDate>
#include <QMap>

// 刷题统计面板 - Codeforces 风格
class PracticeStatsPanel : public QWidget
{
    Q_OBJECT
    
public:
    explicit PracticeStatsPanel(QWidget *parent = nullptr);
    
    // 更新统计数据
    void updateStats(int totalCompleted, int currentStreak, int longestStreak, int todayCompleted);
    void updateHeatMap(const QMap<QDate, int> &activityData);
    void updateDifficultyDistribution(int easyCompleted, int easyTotal,
                                      int mediumCompleted, int mediumTotal,
                                      int hardCompleted, int hardTotal);
    
    // 刷新所有统计数据（从ProgressManager重新加载）
    void refreshStats();
    
private:
    void setupUI();
    void createHeatMap();
    void createDifficultyChart();
    void createStreakInfo();
    void createRecentActivity();
    
    // 获取统计数据
    QMap<QDate, int> getActivityData() const;
    int getCurrentStreak() const;
    int getLongestStreak() const;
    QMap<QString, int> getDifficultyStats() const;
    
    // UI组件
    QWidget *m_heatMapWidget;
    QWidget *m_difficultyWidget;
    QWidget *m_streakWidget;
    QWidget *m_activityWidget;
    
    QLabel *m_totalSolvedLabel;
    QLabel *m_currentStreakLabel;
    QLabel *m_longestStreakLabel;
    QLabel *m_todayCountLabel;
};

#endif // PRACTICESTATSPANEL_H
