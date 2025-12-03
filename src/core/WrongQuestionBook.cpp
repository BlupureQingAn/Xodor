#include "WrongQuestionBook.h"
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

WrongQuestionBook& WrongQuestionBook::instance()
{
    static WrongQuestionBook inst;
    return inst;
}

QString WrongQuestionBook::dataFilePath() const
{
    return "data/wrong_questions.json";
}

void WrongQuestionBook::addWrongQuestion(const Question &question, const QString &userCode, const QString &errorReason)
{
    // 检查是否已存在
    for (auto &record : m_wrongQuestions) {
        if (record.questionId == question.id()) {
            // 更新记录
            record.attemptTime = QDateTime::currentDateTime();
            record.userCode = userCode;
            record.errorReason = errorReason;
            record.attemptCount++;
            save();
            emit wrongQuestionAdded(question.id());
            return;
        }
    }
    
    // 添加新记录
    WrongQuestionRecord record;
    record.questionId = question.id();
    record.questionTitle = question.title();
    record.difficulty = question.difficulty();
    record.attemptTime = QDateTime::currentDateTime();
    record.userCode = userCode;
    record.errorReason = errorReason;
    record.attemptCount = 1;
    record.resolved = false;
    
    m_wrongQuestions.append(record);
    save();
    emit wrongQuestionAdded(question.id());
}

void WrongQuestionBook::markAsResolved(const QString &questionId)
{
    for (auto &record : m_wrongQuestions) {
        if (record.questionId == questionId) {
            record.resolved = true;
            save();
            emit questionResolved(questionId);
            return;
        }
    }
}

QVector<WrongQuestionRecord> WrongQuestionBook::getAllWrongQuestions() const
{
    return m_wrongQuestions;
}

QVector<WrongQuestionRecord> WrongQuestionBook::getUnresolvedQuestions() const
{
    QVector<WrongQuestionRecord> unresolved;
    for (const auto &record : m_wrongQuestions) {
        if (!record.resolved) {
            unresolved.append(record);
        }
    }
    return unresolved;
}

int WrongQuestionBook::getWrongQuestionCount() const
{
    return m_wrongQuestions.size();
}

int WrongQuestionBook::getUnresolvedCount() const
{
    int count = 0;
    for (const auto &record : m_wrongQuestions) {
        if (!record.resolved) {
            count++;
        }
    }
    return count;
}

void WrongQuestionBook::load()
{
    QFile file(dataFilePath());
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    
    if (!doc.isArray()) {
        return;
    }
    
    m_wrongQuestions.clear();
    QJsonArray array = doc.array();
    
    for (const auto &value : array) {
        QJsonObject obj = value.toObject();
        
        WrongQuestionRecord record;
        record.questionId = obj["questionId"].toString();
        record.questionTitle = obj["questionTitle"].toString();
        
        QString diffStr = obj["difficulty"].toString();
        if (diffStr == "easy") record.difficulty = Difficulty::Easy;
        else if (diffStr == "hard") record.difficulty = Difficulty::Hard;
        else record.difficulty = Difficulty::Medium;
        
        record.attemptTime = QDateTime::fromString(obj["attemptTime"].toString(), Qt::ISODate);
        record.userCode = obj["userCode"].toString();
        record.errorReason = obj["errorReason"].toString();
        record.attemptCount = obj["attemptCount"].toInt();
        record.resolved = obj["resolved"].toBool();
        
        m_wrongQuestions.append(record);
    }
}

void WrongQuestionBook::save()
{
    QDir dir("data");
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    
    QJsonArray array;
    for (const auto &record : m_wrongQuestions) {
        QJsonObject obj;
        obj["questionId"] = record.questionId;
        obj["questionTitle"] = record.questionTitle;
        
        QString diffStr = "medium";
        if (record.difficulty == Difficulty::Easy) diffStr = "easy";
        else if (record.difficulty == Difficulty::Hard) diffStr = "hard";
        obj["difficulty"] = diffStr;
        
        obj["attemptTime"] = record.attemptTime.toString(Qt::ISODate);
        obj["userCode"] = record.userCode;
        obj["errorReason"] = record.errorReason;
        obj["attemptCount"] = record.attemptCount;
        obj["resolved"] = record.resolved;
        
        array.append(obj);
    }
    
    QFile file(dataFilePath());
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(array).toJson());
        file.close();
    }
}

void WrongQuestionBook::clear()
{
    m_wrongQuestions.clear();
    save();
}
