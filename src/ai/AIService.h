#ifndef AISERVICE_H
#define AISERVICE_H

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QJsonArray>

class AIService : public QObject
{
    Q_OBJECT
public:
    explicit AIService(QObject *parent = nullptr);
    virtual ~AIService() = default;
    
    virtual void analyzeCode(const QString &questionDesc, const QString &code) = 0;
    virtual void generateQuestions(const QJsonObject &params) = 0;
    virtual void parseQuestionBank(const QStringList &mdFiles) = 0;
    
signals:
    void codeAnalysisReady(const QString &analysis);
    void questionsGenerated(const QJsonArray &questions);
    void questionBankParsed(const QJsonArray &questions);
    void error(const QString &errorMsg);
    
    // 流式响应进度信号
    void streamProgress(const QString &context, int currentLength, const QString &partialContent);
};

#endif // AISERVICE_H
