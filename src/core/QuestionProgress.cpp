#include "QuestionProgress.h"

QJsonObject QuestionProgressRecord::toJson() const
{
    QJsonObject obj;
    obj["questionId"] = questionId;
    obj["status"] = static_cast<int>(status);
    obj["attemptCount"] = attemptCount;
    obj["correctCount"] = correctCount;
    obj["lastAttemptTime"] = lastAttemptTime.toString(Qt::ISODate);
    obj["firstAttemptTime"] = firstAttemptTime.toString(Qt::ISODate);
    obj["lastCode"] = lastCode;
    return obj;
}

QuestionProgressRecord QuestionProgressRecord::fromJson(const QJsonObject &json)
{
    QuestionProgressRecord record;
    record.questionId = json["questionId"].toString();
    record.status = static_cast<QuestionStatus>(json["status"].toInt());
    record.attemptCount = json["attemptCount"].toInt();
    record.correctCount = json["correctCount"].toInt();
    record.lastAttemptTime = QDateTime::fromString(json["lastAttemptTime"].toString(), Qt::ISODate);
    record.firstAttemptTime = QDateTime::fromString(json["firstAttemptTime"].toString(), Qt::ISODate);
    record.lastCode = json["lastCode"].toString();
    return record;
}
