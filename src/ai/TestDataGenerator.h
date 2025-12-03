#ifndef TESTDATAGENERATOR_H
#define TESTDATAGENERATOR_H

#include <QObject>
#include <QVector>
#include "../core/Question.h"

class OllamaClient;

// 测试数据生成器
class TestDataGenerator : public QObject
{
    Q_OBJECT
public:
    explicit TestDataGenerator(OllamaClient *aiClient, QObject *parent = nullptr);
    
    // 为单个题目生成测试数据
    void generateTestData(const Question &question, int additionalCount = 5);
    
    // 批量为题目生成测试数据
    void generateBatchTestData(const QVector<Question> &questions, int additionalCount = 5);
    
signals:
    void testDataGenerated(const QString &questionId, const QVector<TestCase> &testCases);
    void batchProgress(int current, int total, const QString &message);
    void batchComplete();
    void error(const QString &errorMsg);
    
private slots:
    void onAIResponse(const QString &response);
    void onAIError(const QString &error);
    
private:
    QString buildPrompt(const Question &question, int additionalCount);
    QVector<TestCase> parseTestCases(const QString &response);
    void processNextQuestion();
    
    OllamaClient *m_aiClient;
    QVector<Question> m_pendingQuestions;
    int m_currentIndex;
    int m_additionalCount;
    QString m_currentQuestionId;
};

#endif // TESTDATAGENERATOR_H
