#ifndef AIJUDGE_H
#define AIJUDGE_H

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include "../core/Question.h"

class OllamaClient;

class AIJudge : public QObject
{
    Q_OBJECT
public:
    explicit AIJudge(OllamaClient *aiClient, QObject *parent = nullptr);
    
    void judgeCode(const Question &question, const QString &code);
    
signals:
    void judgeStarted();
    void judgeProgress(const QString &status);
    void judgeCompleted(bool passed, const QString &comment, const QVector<int> &failedTestCases);
    void testCaseFixed(int index, const QString &newInput, const QString &newOutput);
    void error(const QString &errorMsg);
    
private slots:
    void onAIResponse(const QString &response);
    void onAIError(const QString &error);
    
private:
    QString buildJudgePrompt(const Question &question, const QString &code);
    void parseJudgeResult(const QString &response);
    
    OllamaClient *m_aiClient;
    Question m_currentQuestion;
    QString m_currentCode;
    QString m_currentResponse;
};

#endif // AIJUDGE_H
