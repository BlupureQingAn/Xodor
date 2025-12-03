#ifndef QUESTIONBANK_H
#define QUESTIONBANK_H

#include <QObject>
#include <QVector>
#include "Question.h"

class QuestionBank : public QObject
{
    Q_OBJECT
public:
    explicit QuestionBank(QObject *parent = nullptr);
    
    void loadFromDirectory(const QString &dirPath);
    void addQuestion(const Question &question);
    void removeQuestion(int index);
    void clear();
    
    QVector<Question> allQuestions() const { return m_questions; }
    Question getQuestion(const QString &id) const;
    int count() const { return m_questions.size(); }
    
signals:
    void questionsLoaded(int count);
    
private:
    QVector<Question> m_questions;
};

#endif // QUESTIONBANK_H
