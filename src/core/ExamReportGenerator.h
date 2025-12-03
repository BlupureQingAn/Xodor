#ifndef EXAMREPORTGENERATOR_H
#define EXAMREPORTGENERATOR_H

#include <QString>
#include <QMap>
#include "ExamSession.h"

// 知识点统计
struct TopicStatistics {
    QString topicName;
    int totalQuestions;
    int correctQuestions;
    double accuracy;
    QStringList weakQuestions;  // 薄弱题目列表
};

// 难度统计
struct DifficultyStatistics {
    Difficulty difficulty;
    int totalQuestions;
    int correctQuestions;
    double accuracy;
    int avgTimeSpent;  // 平均用时（秒）
};

// 答题报告
struct ExamReport {
    // 基本信息
    QString sessionId;
    QString examName;
    QString category;
    QDateTime startTime;
    QDateTime endTime;
    
    // 时间统计
    int totalTimeLimit;  // 总时间限制（分钟）
    int actualTimeSpent;  // 实际用时（分钟）
    bool isTimeout;
    
    // 题目统计
    int totalQuestions;
    int attemptedQuestions;
    int correctQuestions;
    double overallAccuracy;
    int totalScore;
    
    // 详细统计
    QMap<QString, TopicStatistics> topicStats;  // 知识点统计
    QMap<Difficulty, DifficultyStatistics> difficultyStats;  // 难度统计
    
    // 答题详情
    QVector<QuestionAttempt> attempts;
    
    // 薄弱知识点
    QStringList weakTopics;
    
    // 建议
    QString suggestions;
    
    // 生成报告文本
    QString toMarkdown() const;
    QString toHtml() const;
    QJsonObject toJson() const;
};

// 答题报告生成器
class ExamReportGenerator
{
public:
    ExamReportGenerator();
    
    // 生成报告
    ExamReport generateReport(const ExamSession &session);
    
    // 保存报告
    bool saveReport(const ExamReport &report, const QString &filePath);
    bool saveReportAsMarkdown(const ExamReport &report, const QString &filePath);
    bool saveReportAsHtml(const ExamReport &report, const QString &filePath);
    
private:
    // 分析知识点
    void analyzeTopics(const ExamSession &session, ExamReport &report);
    
    // 分析难度
    void analyzeDifficulty(const ExamSession &session, ExamReport &report);
    
    // 识别薄弱知识点
    void identifyWeakTopics(ExamReport &report);
    
    // 生成建议
    QString generateSuggestions(const ExamReport &report);
    
    // 格式化时间
    QString formatTime(int seconds) const;
    QString formatDifficulty(Difficulty diff) const;
};

#endif // EXAMREPORTGENERATOR_H
