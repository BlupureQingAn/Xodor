#ifndef QUESTIONPROGRESS_H
#define QUESTIONPROGRESS_H

#include <QString>
#include <QDateTime>
#include <QJsonObject>

enum class QuestionStatus {
    NotStarted,     // 未开始
    InProgress,     // 进行中
    Completed,      // 已完成
    Mastered        // 已掌握（多次正确）
};

// 单个题目的进度记录
struct QuestionProgressRecord {
    QString questionId;
    QString questionTitle;      // 题目标题（用于显示）
    QuestionStatus status;
    int attemptCount;           // 尝试次数
    int correctCount;           // 正确次数
    QDateTime lastAttemptTime;  // 最后尝试时间
    QDateTime firstAttemptTime; // 首次尝试时间
    QString lastCode;           // 最后提交的代码
    
    // AI判定相关
    bool aiJudgePassed;         // AI判定是否通过
    QDateTime aiJudgeTime;      // AI判定时间
    QString aiJudgeComment;     // AI判定评语
    
    QuestionProgressRecord()
        : status(QuestionStatus::NotStarted)
        , attemptCount(0)
        , correctCount(0)
        , aiJudgePassed(false)
    {}
    
    QJsonObject toJson() const;
    static QuestionProgressRecord fromJson(const QJsonObject &json);
    
    // 计算正确率
    double accuracy() const {
        return attemptCount > 0 ? (double)correctCount / attemptCount * 100.0 : 0.0;
    }
};

#endif // QUESTIONPROGRESS_H
