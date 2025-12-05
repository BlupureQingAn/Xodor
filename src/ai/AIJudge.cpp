#include "AIJudge.h"
#include "OllamaClient.h"
#include <QJsonDocument>
#include <QRegularExpression>
#include <QDebug>

AIJudge::AIJudge(OllamaClient *aiClient, QObject *parent)
    : QObject(parent)
    , m_aiClient(aiClient)
{
}

QString AIJudge::buildJudgePrompt(const Question &question, const QString &code)
{
    QString prompt = QString(R"(
你是一个专业的代码评判专家。请分析以下C++代码是否正确实现了题目要求。

【题目信息】
标题：%1
描述：%2

【学生代码】
```cpp
%3
```

【评判要求】
1. 仔细阅读题目描述，理解题目的核心要求
2. 分析代码逻辑是否正确实现了题目要求
3. 检查算法思路是否正确
4. 检查边界条件处理是否完善
5. 检查输入输出格式是否符合题目要求
6. 不要运行测试用例，只从代码逻辑角度判断

【输出格式】
请以JSON格式输出评判结果：
```json
{
    "passed": true/false,
    "comment": "详细的评判说明，包括：\n1. 代码逻辑分析\n2. 算法正确性评价\n3. 如果不通过，指出具体问题和改进建议"
}
```

请开始评判：
)");

    return prompt.arg(question.title(), question.description(), code);
}

void AIJudge::judgeCode(const Question &question, const QString &code)
{
    if (!m_aiClient) {
        qCritical() << "[AIJudge] ERROR: AI客户端未初始化";
        emit error("AI客户端未初始化");
        return;
    }
    
    qDebug() << "[AIJudge] Starting judge for question:" << question.id() << question.title();
    
    m_currentQuestion = question;
    m_currentCode = code;
    m_currentResponse.clear();
    
    emit judgeStarted();
    emit judgeProgress("正在分析代码...");
    
    QString prompt = buildJudgePrompt(question, code);
    
    qDebug() << "[AIJudge] Prompt length:" << prompt.length();
    
    // 先断开旧的连接，避免重复连接
    disconnect(m_aiClient, &OllamaClient::codeAnalysisReady, this, &AIJudge::onAIResponse);
    disconnect(m_aiClient, &OllamaClient::error, this, &AIJudge::onAIError);
    
    // 连接信号
    connect(m_aiClient, &OllamaClient::codeAnalysisReady, 
            this, &AIJudge::onAIResponse, Qt::UniqueConnection);
    connect(m_aiClient, &OllamaClient::error, 
            this, &AIJudge::onAIError, Qt::UniqueConnection);
    
    qDebug() << "[AIJudge] Sending prompt to AI client...";
    // 使用特殊的context "ai_judge"，避免触发AI导师面板的流式输出
    m_aiClient->sendCustomPrompt(prompt, "ai_judge");
}

void AIJudge::onAIResponse(const QString &response)
{
    qDebug() << "[AIJudge] Received AI response, length:" << response.length();
    
    // 断开信号
    if (m_aiClient) {
        disconnect(m_aiClient, &OllamaClient::codeAnalysisReady, this, &AIJudge::onAIResponse);
        disconnect(m_aiClient, &OllamaClient::error, this, &AIJudge::onAIError);
    }
    
    m_currentResponse = response;
    
    try {
        parseJudgeResult(response);
    } catch (const std::exception &e) {
        qCritical() << "[AIJudge] Exception in parseJudgeResult:" << e.what();
        emit error(QString("解析AI响应时发生错误：%1").arg(e.what()));
    } catch (...) {
        qCritical() << "[AIJudge] Unknown exception in parseJudgeResult";
        emit error("解析AI响应时发生未知错误");
    }
}

void AIJudge::parseJudgeResult(const QString &response)
{
    qDebug() << "[AIJudge] Parsing judge result...";
    
    if (response.isEmpty()) {
        qWarning() << "[AIJudge] Empty response";
        emit error("AI返回了空响应");
        return;
    }
    
    // 提取JSON
    QRegularExpression jsonRegex(R"(```json\s*(\{[\s\S]*?\})\s*```)");
    QRegularExpressionMatch match = jsonRegex.match(response);
    
    if (!match.hasMatch()) {
        qDebug() << "[AIJudge] No JSON block found, trying to find raw JSON...";
        // 尝试直接查找JSON对象
        int jsonStart = response.indexOf('{');
        if (jsonStart >= 0) {
            int jsonEnd = response.lastIndexOf('}');
            if (jsonEnd > jsonStart) {
                QString jsonStr = response.mid(jsonStart, jsonEnd - jsonStart + 1);
                qDebug() << "[AIJudge] Found raw JSON, length:" << jsonStr.length();
                
                QJsonParseError parseError;
                QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8(), &parseError);
                
                if (parseError.error != QJsonParseError::NoError) {
                    qWarning() << "[AIJudge] JSON parse error:" << parseError.errorString();
                    emit error(QString("JSON解析失败：%1").arg(parseError.errorString()));
                    return;
                }
                
                if (doc.isObject()) {
                    QJsonObject result = doc.object();
                    bool passed = result["passed"].toBool();
                    QString comment = result["comment"].toString();
                    
                    if (comment.isEmpty()) {
                        comment = "AI未提供评论";
                    }
                    
                    QVector<int> failedTestCases;
                    if (result.contains("failedTestCases")) {
                        QJsonArray failedArray = result["failedTestCases"].toArray();
                        for (const QJsonValue &val : failedArray) {
                            failedTestCases.append(val.toInt());
                        }
                    }
                    
                    qDebug() << "[AIJudge] Parse success - Passed:" << passed << "Failed cases:" << failedTestCases.size();
                    emit judgeCompleted(passed, comment, failedTestCases);
                    return;
                }
            }
        }
        
        qWarning() << "[AIJudge] No valid JSON found in response";
        emit error("AI响应格式错误：未找到有效的JSON数据");
        return;
    }
    
    QString jsonStr = match.captured(1);
    qDebug() << "[AIJudge] Found JSON block, length:" << jsonStr.length();
    
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8(), &parseError);
    
    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "[AIJudge] JSON parse error:" << parseError.errorString();
        emit error(QString("JSON解析失败：%1").arg(parseError.errorString()));
        return;
    }
    
    if (!doc.isObject()) {
        qWarning() << "[AIJudge] JSON is not an object";
        emit error("JSON格式错误：期望对象类型");
        return;
    }
    
    QJsonObject result = doc.object();
    
    // 检查必需字段
    if (!result.contains("passed")) {
        qWarning() << "[AIJudge] Missing 'passed' field";
        emit error("JSON格式错误：缺少'passed'字段");
        return;
    }
    
    bool passed = result["passed"].toBool();
    QString comment = result.value("comment").toString();
    
    if (comment.isEmpty()) {
        comment = "AI未提供评论";
    }
    
    // 获取失败的测试用例
    QVector<int> failedTestCases;
    if (result.contains("failedTestCases")) {
        QJsonArray failedArray = result["failedTestCases"].toArray();
        for (const QJsonValue &val : failedArray) {
            if (val.isDouble()) {
                failedTestCases.append(val.toInt());
            }
        }
    }
    
    // 处理测试用例问题
    if (result.contains("testCaseIssues")) {
        QJsonArray issuesArray = result["testCaseIssues"].toArray();
        for (const QJsonValue &val : issuesArray) {
            if (!val.isObject()) continue;
            
            QJsonObject issue = val.toObject();
            int index = issue.value("index").toInt();
            QString suggestedInput = issue.value("suggestedInput").toString();
            QString suggestedOutput = issue.value("suggestedOutput").toString();
            
            // 检查是否过长
            if (suggestedInput.length() > 5000 || suggestedOutput.length() > 5000) {
                // 过长，放弃修复
                comment += QString("\n⚠️ 测试用例 %1 的IO数据过长，无法自动修复").arg(index);
            } else if (!suggestedInput.isEmpty() && !suggestedOutput.isEmpty()) {
                emit testCaseFixed(index, suggestedInput, suggestedOutput);
            }
        }
    }
    
    qDebug() << "[AIJudge] Parse complete - Passed:" << passed << "Failed cases:" << failedTestCases.size();
    emit judgeCompleted(passed, comment, failedTestCases);
}

void AIJudge::onAIError(const QString &error)
{
    qWarning() << "[AIJudge] AI error:" << error;
    
    // 断开信号
    if (m_aiClient) {
        disconnect(m_aiClient, &OllamaClient::codeAnalysisReady, this, &AIJudge::onAIResponse);
        disconnect(m_aiClient, &OllamaClient::error, this, &AIJudge::onAIError);
    }
    
    emit this->error(QString("AI判题失败：%1").arg(error));
}
