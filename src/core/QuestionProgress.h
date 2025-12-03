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
    QuestionStatus status;
    int attemptCount;           // 尝试次数
    int correctCount;           // 正确次数
    QDateTime lastAttemptTime;  // 最后尝试时间
    QDateTime firstAttemptTime; // 首次尝试时间
    QString lastCode;           // 最后提交的代码
    
    QuestionProgressRecord()
        : status(QuestionStatus::NotStarted)
        , attemptCount(0)
        , correctCount(0)
    {}
    
    QJsonObject toJson() const;
    static QuestionProgressRecord fromJson(const QJsonObject &json);
    
    // 计算正确率
    double accuracy() const {
        return attemptCount > 0 ? (double)correctCount / attemptCount * 100.0 : 0.0;
    }
};

#endif // QUESTIONPROGRESS_H
