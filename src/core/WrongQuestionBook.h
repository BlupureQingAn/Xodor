#ifndef WRONGQUESTIONBOOK_H
#define WRONGQUESTIONBOOK_H

#include <QObject>
#include <QVector>
#include <QDateTime>
#include "Question.h"

struct WrongQuestionRecord {
    QString questionId;
    QString questionTitle;
    Difficulty difficulty;
    QDateTime attemptTime;
    QString userCode;
    QString errorReason;
    int attemptCount;
    bool resolved;
};

class WrongQuestionBook : public QObject
{
    Q_OBJECT
public:
    static WrongQuestionBook& instance();
    
    void addWrongQuestion(const Question &question, const QString &userCode, const QString &errorReason);
    void markAsResolved(const QString &questionId);
    
    QVector<WrongQuestionRecord> getAllWrongQuestions() const;
    QVector<WrongQuestionRecord> getUnresolvedQuestions() const;
    int getWrongQuestionCount() const;
    int getUnresolvedCount() const;
    
    void load();
    void save();
    void clear();
    
signals:
    void wrongQuestionAdded(const QString &questionId);
    void questionResolved(const QString &questionId);
    
private:
    WrongQuestionBook() = default;
    WrongQuestionBook(const WrongQuestionBook&) = delete;
    WrongQuestionBook& operator=(const WrongQuestionBook&) = delete;
    
    QVector<WrongQuestionRecord> m_wrongQuestions;
    QString dataFilePath() const;
};

#endif // WRONGQUESTIONBOOK_H
