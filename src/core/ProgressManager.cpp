#include "ProgressManager.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QStandardPaths>
#include <QDir>

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
