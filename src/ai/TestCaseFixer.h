#ifndef TESTCASEFIXER_H
#define TESTCASEFIXER_H

#include <QObject>
#include "../core/Question.h"

class OllamaClient;

class TestCaseFixer : public QObject
{
    Q_OBJECT
public:
    explicit TestCaseFixer(OllamaClient *aiClient, QObject *parent = nullptr);
    
    // 检测并修复题目的测试用例
    void fixTestCases(const Question &question, const QString &questionFilePath);
    
signals:
    void fixStarted();
    void fixProgress(const QString &message);
    void fixCompleted(bool success, const QString &message);
    void testCasesFixed(const Question &fixedQuestion);
    
private:
    OllamaClient *m_aiClient;
    
    // 检测测试用例是否有问题
    bool hasTestCaseIssues(const QVector<TestCase> &testCases);
    
    // 生成修复提示词
    QString buildFixPrompt(const Question &question, const QVector<TestCase> &problematicCases);
    
    // 解析AI返回的修复后的测试用例
    QVector<TestCase> parseFixedTestCases(const QString &aiResponse);
};

#endif // TESTCASEFIXER_H
