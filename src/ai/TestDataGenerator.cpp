#include "TestDataGenerator.h"
#include "OllamaClient.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

TestDataGenerator::TestDataGenerator(OllamaClient *aiClient, QObject *parent)
    : QObject(parent)
    , m_aiClient(aiClient)
    , m_currentIndex(0)
    , m_additionalCount(5)
{
    if (m_aiClient) {
        connect(m_aiClient, &OllamaClient::codeAnalysisReady,
                this, &TestDataGenerator::onAIResponse);
        connect(m_aiClient, &OllamaClient::error,
                this, &TestDataGenerator::onAIError);
    }
}

void TestDataGenerator::generateTestData(const Question &question, int additionalCount)
{
    if (!m_aiClient) {
        emit error("AI服务未配置");
        return;
    }
    
    m_currentQuestionId = question.id();
    m_additionalCount = additionalCount;
    
    QString prompt = buildPrompt(question, additionalCount);
    m_aiClient->analyzeCode("", prompt);
}

void TestDataGenerator::generateBatchTestData(const QVector<Question> &questions, int additionalCount)
{
    if (!m_aiClient) {
        emit error("AI服务未配置");
        return;
    }
    
    m_pendingQuestions = questions;
    m_currentIndex = 0;
    m_additionalCount = additionalCount;
    
    if (!m_pendingQuestions.isEmpty()) {
        processNextQuestion();
    }
}

QString TestDataGenerator::buildPrompt(const Question &question, int additionalCount)
{
    QString prompt = R"(
你是一个专业的测试数据生成助手。请为以下编程题目生成 %1 组补充测试数据。

【题目信息】
标题：%2
描述：
%3

【现有测试数据】
%4

【生成要求】
1. 生成 %1 组新的测试数据
2. 覆盖以下场景：
   - 边界条件（最小值、最大值）
   - 异常情况（空输入、特殊字符）
   - 等价类测试（不同类型的有效输入）
   - 性能测试（大数据量）
3. 每组数据包含：
   - input: 输入数据
   - output: 期望输出
   - description: 测试场景描述

【输出格式】
返回纯 JSON 格式：
{
  "testCases": [
    {
      "input": "输入数据",
      "output": "期望输出",
      "description": "边界条件：最小值"
    },
    {
      "input": "输入数据",
      "output": "期望输出",
      "description": "边界条件：最大值"
    }
  ]
}

【注意事项】
- 测试数据要与题目要求严格对应
- 输入输出格式要与题目描述一致
- 不要重复现有测试数据
- 返回纯 JSON，不要包含其他文字

请开始生成。
)";

    // 构建现有测试数据说明
    QString existingTests;
    QVector<TestCase> cases = question.testCases();
    for (int i = 0; i < cases.size() && i < 3; ++i) {
        existingTests += QString("示例 %1：\n输入：%2\n输出：%3\n\n")
            .arg(i + 1)
            .arg(cases[i].input)
            .arg(cases[i].expectedOutput);
    }
    
    if (existingTests.isEmpty()) {
        existingTests = "（无现有测试数据）";
    }
    
    prompt = prompt
        .arg(additionalCount)
        .arg(question.title())
        .arg(question.description())
        .arg(existingTests);
    
    return prompt;
}

QVector<TestCase> TestDataGenerator::parseTestCases(const QString &response)
{
    QVector<TestCase> testCases;
    
    // 提取 JSON
    QString jsonStr = response;
    
    int jsonStart = response.indexOf("```json");
    if (jsonStart >= 0) {
        jsonStart = response.indexOf('\n', jsonStart) + 1;
        int jsonEnd = response.indexOf("```", jsonStart);
        if (jsonEnd > jsonStart) {
            jsonStr = response.mid(jsonStart, jsonEnd - jsonStart).trimmed();
        }
    } else {
        jsonStart = response.indexOf('{');
        if (jsonStart >= 0) {
            jsonStr = response.mid(jsonStart);
        }
    }
    
    // 解析 JSON
    QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());
    if (doc.isNull() || !doc.isObject()) {
        return testCases;
    }
    
    QJsonObject root = doc.object();
    QJsonArray casesArray = root["testCases"].toArray();
    
    for (const QJsonValue &val : casesArray) {
        QJsonObject caseObj = val.toObject();
        TestCase tc;
        tc.input = caseObj["input"].toString();
        tc.expectedOutput = caseObj["output"].toString();
        tc.description = caseObj["description"].toString();
        tc.isAIGenerated = true;
        testCases.append(tc);
    }
    
    return testCases;
}

void TestDataGenerator::processNextQuestion()
{
    if (m_currentIndex >= m_pendingQuestions.size()) {
        emit batchComplete();
        return;
    }
    
    const Question &question = m_pendingQuestions[m_currentIndex];
    m_currentQuestionId = question.id();
    
    emit batchProgress(m_currentIndex + 1, m_pendingQuestions.size(),
                      QString("正在为题目 \"%1\" 生成测试数据...").arg(question.title()));
    
    QString prompt = buildPrompt(question, m_additionalCount);
    m_aiClient->analyzeCode("", prompt);
}

void TestDataGenerator::onAIResponse(const QString &response)
{
    QVector<TestCase> testCases = parseTestCases(response);
    
    if (!testCases.isEmpty()) {
        emit testDataGenerated(m_currentQuestionId, testCases);
    }
    
    // 如果是批量处理，继续下一个
    if (!m_pendingQuestions.isEmpty()) {
        m_currentIndex++;
        processNextQuestion();
    }
}

void TestDataGenerator::onAIError(const QString &error)
{
    emit this->error(QString("测试数据生成错误: %1").arg(error));
    
    // 批量处理时跳过错误继续
    if (!m_pendingQuestions.isEmpty()) {
        m_currentIndex++;
        processNextQuestion();
    }
}
