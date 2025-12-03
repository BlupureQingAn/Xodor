#include "ExamSession.h"
#include <QJsonArray>
#include <QUuid>

// QuestionAttempt 实现
QJsonObject QuestionAttempt::toJson() const
{
    QJsonObject obj;
    obj["questionId"] = questionId;
    obj["questionTitle"] = questionTitle;
    obj["difficulty"] = static_cast<int>(difficulty);
    obj["tags"] = QJsonArray::fromStringList(tags);
    obj["submittedCode"] = submittedCode;
    obj["startTime"] = startTime.toString(Qt::ISODate);
    obj["submitTime"] = submitTime.toString(Qt::ISODate);
    obj["timeSpent"] = timeSpent;
    obj["totalTestCases"] = totalTestCases;
    obj["passedTestCases"] = passedTestCases;
    obj["isCorrect"] = isCorrect;
    obj["errorMessage"] = errorMessage;
    return obj;
}

QuestionAttempt QuestionAttempt::fromJson(const QJsonObject &json)
{
    QuestionAttempt attempt;
    attempt.questionId = json["questionId"].toString();
    attempt.questionTitle = json["questionTitle"].toString();
    attempt.difficulty = static_cast<Difficulty>(json["difficulty"].toInt());
    
    QJsonArray tagsArray = json["tags"].toArray();
    for (const QJsonValue &val : tagsArray) {
        attempt.tags.append(val.toString());
    }
    
    attempt.submittedCode = json["submittedCode"].toString();
    attempt.startTime = QDateTime::fromString(json["startTime"].toString(), Qt::ISODate);
    attempt.submitTime = QDateTime::fromString(json["submitTime"].toString(), Qt::ISODate);
    attempt.timeSpent = json["timeSpent"].toInt();
    attempt.totalTestCases = json["totalTestCases"].toInt();
    attempt.passedTestCases = json["passedTestCases"].toInt();
    attempt.isCorrect = json["isCorrect"].toBool();
    attempt.errorMessage = json["errorMessage"].toString();
    
    return attempt;
}

// ExamSession 实现
ExamSession::ExamSession()
    : m_totalTimeLimit(180)
{
    m_sessionId = QUuid::createUuid().toString(QUuid::WithoutBraces);
}

ExamSession::ExamSession(const QString &examName, const QVector<Question> &questions)
    : m_examName(examName)
    , m_questions(questions)
    , m_totalTimeLimit(180)
{
    m_sessionId = QUuid::createUuid().toString(QUuid::WithoutBraces);
}

int ExamSession::timeSpent() const
{
    if (!isStarted()) return 0;
    
    QDateTime end = isFinished() ? m_endTime : QDateTime::currentDateTime();
    return m_startTime.secsTo(end) / 60;  // 转换为分钟
}

bool ExamSession::isTimeout() const
{
    return timeSpent() > m_totalTimeLimit;
}

void ExamSession::addAttempt(const QuestionAttempt &attempt)
{
    m_attempts.append(attempt);
}

void ExamSession::start()
{
    if (!isStarted()) {
        m_startTime = QDateTime::currentDateTime();
    }
}

void ExamSession::finish()
{
    if (!isFinished()) {
        m_endTime = QDateTime::currentDateTime();
    }
}

int ExamSession::correctCount() const
{
    int count = 0;
    for (const QuestionAttempt &attempt : m_attempts) {
        if (attempt.isCorrect) {
            count++;
        }
    }
    return count;
}

int ExamSession::totalScore() const
{
    int score = 0;
    for (const QuestionAttempt &attempt : m_attempts) {
        if (attempt.isCorrect) {
            // 根据难度给分
            switch (attempt.difficulty) {
                case Difficulty::Easy:
                    score += 10;
                    break;
                case Difficulty::Medium:
                    score += 20;
                    break;
                case Difficulty::Hard:
                    score += 30;
                    break;
            }
        }
    }
    return score;
}

double ExamSession::accuracy() const
{
    if (m_attempts.isEmpty()) return 0.0;
    return (double)correctCount() / m_attempts.size() * 100.0;
}

QJsonObject ExamSession::toJson() const
{
    QJsonObject obj;
    obj["sessionId"] = m_sessionId;
    obj["examName"] = m_examName;
    obj["category"] = m_category;
    obj["startTime"] = m_startTime.toString(Qt::ISODate);
    obj["endTime"] = m_endTime.toString(Qt::ISODate);
    obj["totalTimeLimit"] = m_totalTimeLimit;
    
    QJsonArray questionsArray;
    for (const Question &q : m_questions) {
        questionsArray.append(q.toJson());
    }
    obj["questions"] = questionsArray;
    
    QJsonArray attemptsArray;
    for (const QuestionAttempt &attempt : m_attempts) {
        attemptsArray.append(attempt.toJson());
    }
    obj["attempts"] = attemptsArray;
    
    return obj;
}

ExamSession ExamSession::fromJson(const QJsonObject &json)
{
    ExamSession session;
    session.m_sessionId = json["sessionId"].toString();
    session.m_examName = json["examName"].toString();
    session.m_category = json["category"].toString();
    session.m_startTime = QDateTime::fromString(json["startTime"].toString(), Qt::ISODate);
    session.m_endTime = QDateTime::fromString(json["endTime"].toString(), Qt::ISODate);
    session.m_totalTimeLimit = json["totalTimeLimit"].toInt(180);
    
    QJsonArray questionsArray = json["questions"].toArray();
    for (const QJsonValue &val : questionsArray) {
        session.m_questions.append(Question(val.toObject()));
    }
    
    QJsonArray attemptsArray = json["attempts"].toArray();
    for (const QJsonValue &val : attemptsArray) {
        session.m_attempts.append(QuestionAttempt::fromJson(val.toObject()));
    }
    
    return session;
}
