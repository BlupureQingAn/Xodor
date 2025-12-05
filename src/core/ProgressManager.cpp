#include "ProgressManager.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QStandardPaths>
#include <QDir>
#include <QDate>
#include <QSet>
#include <algorithm>

ProgressManager& ProgressManager::instance()
{
    static ProgressManager instance;
    return instance;
}

ProgressManager::ProgressManager(QObject *parent)
    : QObject(parent)
{
    load();
}

ProgressManager::~ProgressManager()
{
    save();
}

QString ProgressManager::getProgressFilePath() const
{
    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(dataPath);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    return dataPath + "/question_progress.json";
}

void ProgressManager::load()
{
    QString filePath = getProgressFilePath();
    QFile file(filePath);
    
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    
    if (!doc.isArray()) {
        return;
    }
    
    m_progressMap.clear();
    QJsonArray arr = doc.array();
    
    for (const auto &val : arr) {
        QuestionProgressRecord record = QuestionProgressRecord::fromJson(val.toObject());
        m_progressMap[record.questionId] = record;
    }
}

void ProgressManager::save()
{
    QJsonArray arr;
    
    for (const auto &record : m_progressMap) {
        arr.append(record.toJson());
    }
    
    QJsonDocument doc(arr);
    
    QString filePath = getProgressFilePath();
    QFile file(filePath);
    
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        file.close();
    }
}

QuestionProgressRecord ProgressManager::getProgress(const QString &questionId) const
{
    if (m_progressMap.contains(questionId)) {
        return m_progressMap[questionId];
    }
    
    QuestionProgressRecord record;
    record.questionId = questionId;
    return record;
}

void ProgressManager::recordAttempt(const QString &questionId, bool correct, const QString &code)
{
    QuestionProgressRecord record = getProgress(questionId);
    
    record.attemptCount++;
    if (correct) {
        record.correctCount++;
    }
    
    record.lastAttemptTime = QDateTime::currentDateTime();
    
    if (record.firstAttemptTime.isNull()) {
        record.firstAttemptTime = record.lastAttemptTime;
    }
    
    if (!code.isEmpty()) {
        record.lastCode = code;
    }
    
    // 自动更新状态
    if (record.status == QuestionStatus::NotStarted) {
        record.status = QuestionStatus::InProgress;
    }
    
    // 如果连续3次正确，标记为已掌握
    if (correct && record.correctCount >= 3 && record.accuracy() >= 75.0) {
        record.status = QuestionStatus::Mastered;
    } else if (correct) {
        record.status = QuestionStatus::Completed;
    }
    
    m_progressMap[questionId] = record;
    
    emit progressUpdated(questionId);
    emit statisticsChanged();
    
    save();
}

void ProgressManager::updateStatus(const QString &questionId, QuestionStatus status)
{
    QuestionProgressRecord record = getProgress(questionId);
    record.status = status;
    m_progressMap[questionId] = record;
    
    emit progressUpdated(questionId);
    emit statisticsChanged();
    
    save();
}

void ProgressManager::saveLastCode(const QString &questionId, const QString &code)
{
    QuestionProgressRecord record = getProgress(questionId);
    record.lastCode = code;
    m_progressMap[questionId] = record;
    save();
}

void ProgressManager::setQuestionTitle(const QString &questionId, const QString &title)
{
    QuestionProgressRecord record = getProgress(questionId);
    record.questionTitle = title;
    m_progressMap[questionId] = record;
    save();
}

int ProgressManager::getCompletedCount() const
{
    int count = 0;
    for (const auto &record : m_progressMap) {
        if (record.status == QuestionStatus::Completed || 
            record.status == QuestionStatus::Mastered) {
            count++;
        }
    }
    return count;
}

int ProgressManager::getMasteredCount() const
{
    int count = 0;
    for (const auto &record : m_progressMap) {
        if (record.status == QuestionStatus::Mastered) {
            count++;
        }
    }
    return count;
}

double ProgressManager::getOverallAccuracy() const
{
    int totalAttempts = 0;
    int totalCorrect = 0;
    
    for (const auto &record : m_progressMap) {
        totalAttempts += record.attemptCount;
        totalCorrect += record.correctCount;
    }
    
    return totalAttempts > 0 ? (double)totalCorrect / totalAttempts * 100.0 : 0.0;
}

QStringList ProgressManager::getQuestionsByStatus(QuestionStatus status) const
{
    QStringList result;
    for (const auto &record : m_progressMap) {
        if (record.status == status) {
            result.append(record.questionId);
        }
    }
    return result;
}

void ProgressManager::clear()
{
    m_progressMap.clear();
    save();
    emit statisticsChanged();
}

void ProgressManager::clearQuestion(const QString &questionId)
{
    m_progressMap.remove(questionId);
    save();
    emit progressUpdated(questionId);
    emit statisticsChanged();
}

void ProgressManager::recordAIJudge(const QString &questionId, bool passed, const QString &comment)
{
    QuestionProgressRecord record = getProgress(questionId);
    
    // 记录AI判定结果
    record.aiJudgePassed = passed;
    record.aiJudgeTime = QDateTime::currentDateTime();
    record.aiJudgeComment = comment;
    
    // 增加尝试次数和正确次数（AI判题也算一次尝试）
    record.attemptCount++;
    if (passed) {
        record.correctCount++;
    }
    
    // 更新时间
    record.lastAttemptTime = QDateTime::currentDateTime();
    if (record.firstAttemptTime.isNull()) {
        record.firstAttemptTime = record.lastAttemptTime;
    }
    
    // 更新状态
    if (record.status == QuestionStatus::NotStarted) {
        record.status = QuestionStatus::InProgress;
    }
    
    if (passed) {
        // AI判定通过，更新状态为已完成
        // 如果连续多次正确，标记为已掌握
        if (record.correctCount >= 3 && record.accuracy() >= 75.0) {
            record.status = QuestionStatus::Mastered;
        } else {
            record.status = QuestionStatus::Completed;
        }
    }
    
    m_progressMap[questionId] = record;
    
    emit progressUpdated(questionId);
    emit statisticsChanged();
    
    save();
}

bool ProgressManager::isAIJudgePassed(const QString &questionId) const
{
    if (m_progressMap.contains(questionId)) {
        return m_progressMap[questionId].aiJudgePassed;
    }
    return false;
}

// 刷题统计方法实现

int ProgressManager::getTotalCompleted() const
{
    int count = 0;
    for (const auto &record : m_progressMap) {
        if (record.status == QuestionStatus::Completed || 
            record.status == QuestionStatus::Mastered) {
            count++;
        }
    }
    return count;
}

int ProgressManager::getCurrentStreak() const
{
    QDate today = QDate::currentDate();
    int streak = 0;
    
    // 从今天开始往前查找连续的日期
    QDate checkDate = today;
    while (true) {
        bool hasActivity = false;
        
        // 检查这一天是否有完成的题目
        for (const auto &record : m_progressMap) {
            if (record.lastAttemptTime.isValid()) {
                QDate attemptDate = record.lastAttemptTime.date();
                if (attemptDate == checkDate && 
                    (record.status == QuestionStatus::Completed || 
                     record.status == QuestionStatus::Mastered)) {
                    hasActivity = true;
                    break;
                }
            }
        }
        
        if (hasActivity) {
            streak++;
            checkDate = checkDate.addDays(-1);
        } else {
            // 如果今天没有活动，允许一次中断
            if (checkDate == today) {
                checkDate = checkDate.addDays(-1);
                continue;
            }
            break;
        }
    }
    
    return streak;
}

int ProgressManager::getLongestStreak() const
{
    if (m_progressMap.isEmpty()) {
        return 0;
    }
    
    // 收集所有完成日期
    QSet<QDate> completedDates;
    for (const auto &record : m_progressMap) {
        if (record.lastAttemptTime.isValid() && 
            (record.status == QuestionStatus::Completed || 
             record.status == QuestionStatus::Mastered)) {
            completedDates.insert(record.lastAttemptTime.date());
        }
    }
    
    if (completedDates.isEmpty()) {
        return 0;
    }
    
    // 转换为排序列表
    QList<QDate> sortedDates = completedDates.values();
    std::sort(sortedDates.begin(), sortedDates.end());
    
    int maxStreak = 1;
    int currentStreak = 1;
    
    for (int i = 1; i < sortedDates.size(); ++i) {
        if (sortedDates[i].daysTo(sortedDates[i-1]) == -1) {
            // 连续的日期
            currentStreak++;
            maxStreak = qMax(maxStreak, currentStreak);
        } else {
            // 不连续，重置
            currentStreak = 1;
        }
    }
    
    return maxStreak;
}

int ProgressManager::getTodayCompleted() const
{
    QDate today = QDate::currentDate();
    int count = 0;
    
    for (const auto &record : m_progressMap) {
        if (record.lastAttemptTime.isValid() && 
            record.lastAttemptTime.date() == today &&
            (record.status == QuestionStatus::Completed || 
             record.status == QuestionStatus::Mastered)) {
            count++;
        }
    }
    
    return count;
}

QMap<QDate, int> ProgressManager::getActivityByDate(int days) const
{
    QMap<QDate, int> activityMap;
    QDate endDate = QDate::currentDate();
    QDate startDate = endDate.addDays(-days + 1);
    
    // 初始化所有日期为0
    for (QDate date = startDate; date <= endDate; date = date.addDays(1)) {
        activityMap[date] = 0;
    }
    
    // 统计每天完成的题目数
    for (const auto &record : m_progressMap) {
        if (record.lastAttemptTime.isValid()) {
            QDate attemptDate = record.lastAttemptTime.date();
            if (attemptDate >= startDate && attemptDate <= endDate &&
                (record.status == QuestionStatus::Completed || 
                 record.status == QuestionStatus::Mastered)) {
                activityMap[attemptDate]++;
            }
        }
    }
    
    return activityMap;
}

QMap<QString, int> ProgressManager::getDifficultyDistribution() const
{
    QMap<QString, int> distribution;
    distribution["easy"] = 0;
    distribution["medium"] = 0;
    distribution["hard"] = 0;
    
    // 注意：这里需要从题目信息中获取难度
    // 由于ProgressManager只存储进度，不存储题目信息
    // 这个方法需要配合QuestionBank使用
    // 暂时返回空分布，在PracticeWidget中实现
    
    return distribution;
}
