#include "TestCaseFixer.h"
#include "OllamaClient.h"
#include "../core/Question.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QRegularExpression>

TestCaseFixer::TestCaseFixer(OllamaClient *aiClient, QObject *parent)
    : QObject(parent)
    , m_aiClient(aiClient)
{
}

void TestCaseFixer::fixTestCases(const Question &question, const QString &questionFilePath)
{
    emit fixStarted();
    emit fixProgress("正在检测测试用例...");
    
    QVector<TestCase> testCases = question.testCases();
    
    // 检测是否有问题
    if (!hasTestCaseIssues(testCases)) {
        emit fixCompleted(true, "测试用例没有发现问题");
        return;
    }
    
    emit fixProgress("发现测试用例问题，正在使用AI修复...");
    
    // 找出有问题的测试用例
    QVector<TestCase> problematicCases;
    for (const TestCase &tc : testCases) {
        bool hasIssue = false;
        
        // 检测输入问题
        if (tc.input.contains("...") || 
            tc.input.contains("（重复") || 
            tc.input.contains("(重复") ||
            tc.input.contains("（") ||
            tc.input.contains("）") ||
            (tc.isAIGenerated && tc.input.length() < 10)) {
            hasIssue = true;
        }
        
        // 检测输出问题
        if (tc.expectedOutput.contains("...") ||
            tc.expectedOutput.contains("（重复") ||
            tc.expectedOutput.contains("(重复") ||
            tc.expectedOutput.contains("（") ||
            tc.expectedOutput.contains("）") ||
            tc.expectedOutput.trimmed().isEmpty() ||
            (tc.isAIGenerated && tc.expectedOutput.length() < 2)) {
            hasIssue = true;
        }
        
        if (hasIssue) {
            problematicCases.append(tc);
        }
    }
    
    // 构建修复提示词
    QString prompt = buildFixPrompt(question, problematicCases);
    
    // 调用AI修复
    if (!m_aiClient) {
        emit fixCompleted(false, "AI客户端未初始化");
        return;
    }
    
    // 使用同步方式获取AI响应（简化实现）
    // 实际应该使用异步，但这里为了简单起见使用同步
    emit fixProgress("等待AI响应...");
    
    // 这里需要实现一个同步调用或者异步回调
    // 暂时先输出提示词，让用户知道需要做什么
    emit fixProgress(QString("AI提示词已生成，请手动修复或等待AI响应\n\n%1").arg(prompt));
    emit fixCompleted(false, "需要实现AI同步调用");
}

bool TestCaseFixer::hasTestCaseIssues(const QVector<TestCase> &testCases)
{
    for (const TestCase &tc : testCases) {
        // 检测输入问题
        if (tc.input.contains("...") || 
            tc.input.contains("（重复") || 
            tc.input.contains("(重复") ||
            tc.input.contains("（") ||
            tc.input.contains("）")) {
            return true;
        }
        
        // 检测输入是否过短（可能不完整）
        if (tc.isAIGenerated && tc.input.length() < 10) {
            return true;
        }
        
        // 检测输出问题
        if (tc.expectedOutput.contains("...") ||
            tc.expectedOutput.contains("（重复") ||
            tc.expectedOutput.contains("(重复") ||
            tc.expectedOutput.contains("（") ||
            tc.expectedOutput.contains("）")) {
            return true;
        }
        
        // 检测输出是否为空
        if (tc.expectedOutput.trimmed().isEmpty()) {
            return true;
        }
        
        // 检测输出是否过短
        if (tc.isAIGenerated && tc.expectedOutput.length() < 2) {
            return true;
        }
    }
    
    return false;
}

QString TestCaseFixer::buildFixPrompt(const Question &question, const QVector<TestCase> &problematicCases)
{
    QString prompt = QString(R"(你是一个测试用例修复专家。请修复以下题目的测试用例。

【题目信息】
标题：%1
描述：%2

【有问题的测试用例】
)").arg(question.title(), question.description());
    
    for (int i = 0; i < problematicCases.size(); ++i) {
        const TestCase &tc = problematicCases[i];
        prompt += QString("\n测试用例 %1：\n").arg(i + 1);
        prompt += QString("描述：%1\n").arg(tc.description);
        prompt += QString("输入：%1\n").arg(tc.input);
        prompt += QString("期望输出：%1\n").arg(tc.expectedOutput);
    }
    
    prompt += R"(

⚠️ 【修复要求 - 非常重要！】
1. 必须生成完整的、可直接使用的实际数据！
2. 将所有省略号（...）、"重复"标记、文字描述展开为完整的实际数据
3. 绝对禁止在修复后的数据中包含：
   - 省略号：...、...（重复N次）、...重复
   - 文字描述：（此处省略）、（重复n次）、（数据过长省略）
   - 符号代替：[...]、<省略>、【略】
4. 如果原数据描述说100行，必须生成完整的100行数据
5. 输入输出格式必须严格符合题目要求
6. 每个测试用例都必须可以直接复制粘贴到程序中运行

❌ 错误示例（修复后仍有问题）：
input: "100 1\n1 0\n...（重复98行）\n0 0"

✅ 正确示例（完整的实际数据）：
input: "5 1\n1 0\n1 0\n1 0\n1 0\n0 0"

【输出格式】
请以JSON格式输出修复后的测试用例，格式如下：
```json
[
    {
        "description": "测试用例描述",
        "input": "完整的实际输入数据（用\\n表示换行）",
        "output": "完整的实际输出数据",
        "isAIGenerated": true
    }
]
```

记住：修复后的数据必须是完整的实际数据，不能有任何省略！

请开始修复：)";
    
    return prompt;
}

QVector<TestCase> TestCaseFixer::parseFixedTestCases(const QString &aiResponse)
{
    QVector<TestCase> fixedCases;
    
    // 提取JSON部分
    QRegularExpression jsonRegex(R"(```json\s*(\[[\s\S]*?\])\s*```)");
    QRegularExpressionMatch match = jsonRegex.match(aiResponse);
    
    if (!match.hasMatch()) {
        return fixedCases;
    }
    
    QString jsonStr = match.captured(1);
    QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());
    
    if (!doc.isArray()) {
        return fixedCases;
    }
    
    QJsonArray array = doc.array();
    for (const QJsonValue &val : array) {
        if (!val.isObject()) continue;
        
        QJsonObject obj = val.toObject();
        TestCase tc;
        tc.description = obj["description"].toString();
        tc.input = obj["input"].toString();
        tc.expectedOutput = obj["output"].toString();
        tc.isAIGenerated = obj["isAIGenerated"].toBool(true);
        
        fixedCases.append(tc);
    }
    
    return fixedCases;
}
