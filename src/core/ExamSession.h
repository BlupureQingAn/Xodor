#ifndef EXAMSESSION_H
#define EXAMSESSION_H

#include <QString>
#include <QDateTime>
#include <QVector>
#include <QJsonObject>
#include "Question.h"

// 单题答题记录
struct QuestionAttempt {
    QString questionId;
    QString questionTitle;
    Difficulty difficulty;
    QStringList tags;
    
    QString submittedCode;          // 提交的代码
    QDateTime startTime;            // 开始时间
    QDateTime submitTime;           // 提交时间
    int timeSpent;                  // 用时（秒）
    
    int totalTestCases;             // 总测试用例数
    int passedTestCases;            // 通过的测试用例数
    bool isCorrect;                 // 是否完全正确
    
    QString errorMessage;           // 错误信息
    
    QJsonObject toJson() const;
    static QuestionAttempt fromJson(const QJsonObject &json);
    
    // 计算通过率
    double passRate() const {
        return totalTestCases > 0 ? (double)passedTestCases / totalTestCases * 100.0 : 0.0;
    }
};

// 套题答题会话
class ExamSession
{
public:
    ExamSession();
    explicit ExamSession(const QString &examName, const QVector<Question> &questions);
    
    // 基本信息
    QString sessionId() const { return m_sessionId; }
    QString examName() const { return m_examName; }
    QString category() const { return m_category; }
    
    // 时间信息
    QDateTime startTime() const { return m_startTime; }
    QDateTime endTime() const { return m_endTime; }
    int totalTimeLimit() const { return m_totalTimeLimit; }  // 总时间限制（分钟）
    int timeSpent() const;  // 实际用时（分钟）
    bool isTimeout() const;
    
    // 题目信息
    int totalQuestions() const { return m_questions.size(); }
    QVector<Question> questions() const { return m_questions; }
    
    // 答题记录
    QVector<QuestionAttempt> attempts() const { return m_attempts; }
    void addAttempt(const QuestionAttempt &attempt);
    
    // 状态
    bool isStarted() const { return !m_startTime.isNull(); }
    bool isFinished() const { return !m_endTime.isNull(); }
    void start();
    void finish();
    
    // 统计信息
    int correctCount() const;
    int totalScore() const;
    double accuracy() const;
    
    // 序列化
    QJsonObject toJson() const;
    static ExamSession fromJson(const QJsonObject &json);
    
    // 设置
    void setCategory(const QString &category) { m_category = category; }
    void setTotalTimeLimit(int minutes) { m_totalTimeLimit = minutes; }
    
private:
    QString m_sessionId;
    QString m_examName;
    QString m_category;
    
    QDateTime m_startTime;
    QDateTime m_endTime;
    int m_totalTimeLimit;  // 分钟
    
    QVector<Question> m_questions;
    QVector<QuestionAttempt> m_attempts;
};

#endif // EXAMSESSION_H
