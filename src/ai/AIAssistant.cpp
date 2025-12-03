#include "AIAssistant.h"
#include "OllamaClient.h"
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QDebug>

AIAssistant::AIAssistant(OllamaClient *aiClient, QObject *parent)
    : QObject(parent)
    , m_aiClient(aiClient)
{
    if (m_aiClient) {
        connect(m_aiClient, &OllamaClient::codeAnalysisReady,
                this, &AIAssistant::onAIResponse);
        connect(m_aiClient, &OllamaClient::error,
                this, &AIAssistant::onAIError);
    }
}

void AIAssistant::askQuestion(const QString &question, const Question &currentQuestion)
{
    if (!m_aiClient) {
        emit error("AI 客户端未初始化");
        return;
    }
    
    m_currentQuestionId = currentQuestion.id();
    
    // 添加用户消息到历史
    addMessage("user", question);
    
    // 构建提示词
    QString prompt = buildQuestionPrompt(question, currentQuestion);
    
    // 发送请求
    m_aiClient->analyzeCode("", prompt);
}

void AIAssistant::getHint(const Question &currentQuestion)
{
    if (!m_aiClient) {
        emit error("AI 客户端未初始化");
        return;
    }
    
    m_currentQuestionId = currentQuestion.id();
    
    // 添加用户消息
    addMessage("user", "请给我一些思路提示");
    
    // 构建提示词
    QString prompt = buildHintPrompt(currentQuestion);
    
    // 发送请求
    m_aiClient->analyzeCode("", prompt);
}

void AIAssistant::explainConcept(const QString &concept, const Question &currentQuestion)
{
    if (!m_aiClient) {
        emit error("AI 客户端未初始化");
        return;
    }
    
    m_currentQuestionId = currentQuestion.id();
    
    // 添加用户消息
    addMessage("user", QString("请讲解一下：%1").arg(concept));
    
    // 构建提示词
    QString prompt = buildConceptPrompt(concept, currentQuestion);
    
    // 发送请求
    m_aiClient->analyzeCode("", prompt);
}

void AIAssistant::diagnoseError(const QString &code, const QString &errorMessage, 
                               const Question &currentQuestion)
{
    if (!m_aiClient) {
        emit error("AI 客户端未初始化");
        return;
    }
    
    m_currentQuestionId = currentQuestion.id();
    
    // 添加用户消息
    addMessage("user", QString("我的代码出错了：%1").arg(errorMessage));
    
    // 构建提示词
    QString prompt = buildDiagnosePrompt(code, errorMessage, currentQuestion);
    
    // 发送请求
    m_aiClient->analyzeCode(code, prompt);
}

void AIAssistant::clearHistory()
{
    m_chatHistory.clear();
}

void AIAssistant::saveHistory(const QString &questionId)
{
    QString filePath = getHistoryFilePath(questionId);
    
    QJsonArray array;
    for (const ChatMessage &msg : m_chatHistory) {
        QJsonObject obj;
        obj["role"] = msg.role;
        obj["content"] = msg.content;
        obj["timestamp"] = msg.timestamp.toString(Qt::ISODate);
        array.append(obj);
    }
    
    QJsonDocument doc(array);
    
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        file.write(doc.toJson());
        file.close();
        qDebug() << "Chat history saved for question:" << questionId;
    }
}

void AIAssistant::loadHistory(const QString &questionId)
{
    QString filePath = getHistoryFilePath(questionId);
    
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        // 文件不存在，清空历史
        m_chatHistory.clear();
        return;
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    
    if (!doc.isArray()) {
        m_chatHistory.clear();
        return;
    }
    
    m_chatHistory.clear();
    QJsonArray array = doc.array();
    
    for (const QJsonValue &val : array) {
        QJsonObject obj = val.toObject();
        ChatMessage msg;
        msg.role = obj["role"].toString();
        msg.content = obj["content"].toString();
        msg.timestamp = QDateTime::fromString(obj["timestamp"].toString(), Qt::ISODate);
        m_chatHistory.append(msg);
    }
    
    qDebug() << "Chat history loaded for question:" << questionId 
             << "Messages:" << m_chatHistory.size();
}

void AIAssistant::onAIResponse(const QString &response)
{
    // 添加 AI 回复到历史
    addMessage("assistant", response);
    
    // 保存历史
    if (!m_currentQuestionId.isEmpty()) {
        saveHistory(m_currentQuestionId);
    }
    
    emit responseReady(response);
}

void AIAssistant::onAIError(const QString &errorMsg)
{
    emit error(errorMsg);
}

QString AIAssistant::buildQuestionPrompt(const QString &userInput, const Question &question)
{
    QString prompt = QString(R"(
你是一位编程导师，正在帮助学生解决编程题目。

【题目】
%1

【题目描述】
%2

【学生的问题】
%3

请回答学生的问题，提供清晰的解释和指导。注意：
1. 不要直接给出完整代码
2. 引导学生思考
3. 可以给出伪代码或关键步骤
4. 语言简洁明了
)").arg(question.title())
   .arg(question.description())
   .arg(userInput);
    
    return prompt;
}

QString AIAssistant::buildHintPrompt(const Question &question)
{
    QString prompt = QString(R"(
你是一位编程导师，正在帮助学生解决编程题目。

【题目】
%1

【题目描述】
%2

请给学生一些解题思路提示。要求：
1. 不要给出具体代码
2. 说明可以使用什么算法或数据结构
3. 说明解题的关键步骤
4. 提示时间和空间复杂度
5. 语言简洁明了

格式：
算法：[算法名称]
思路：[解题思路]
复杂度：时间 O(?) 空间 O(?)
关键点：[需要注意的地方]
)").arg(question.title())
   .arg(question.description());
    
    return prompt;
}

QString AIAssistant::buildConceptPrompt(const QString &concept, const Question &question)
{
    QString prompt = QString(R"(
你是一位编程导师，正在帮助学生理解编程概念。

【当前题目】
%1

【学生想了解的概念】
%2

请用简单易懂的语言讲解这个概念，并结合当前题目说明如何应用。要求：
1. 概念定义清晰
2. 举例说明
3. 结合题目场景
4. 语言通俗易懂
)").arg(question.title())
   .arg(concept);
    
    return prompt;
}

QString AIAssistant::buildDiagnosePrompt(const QString &code, const QString &errorMessage, 
                                        const Question &question)
{
    QString prompt = QString(R"(
你是一位编程导师，正在帮助学生调试代码。

【题目】
%1

【学生的代码】
```cpp
%2
```

【错误信息】
%3

请分析错误原因并给出修改建议。要求：
1. 指出错误的具体位置
2. 解释为什么会出错
3. 给出修改建议（不要直接给完整代码）
4. 提示需要注意的边界条件
5. 语言简洁明了
)").arg(question.title())
   .arg(code)
   .arg(errorMessage);
    
    return prompt;
}

void AIAssistant::addMessage(const QString &role, const QString &content)
{
    ChatMessage msg;
    msg.role = role;
    msg.content = content;
    msg.timestamp = QDateTime::currentDateTime();
    
    m_chatHistory.append(msg);
    
    // 限制历史消息数量（保留最近 20 条）
    if (m_chatHistory.size() > 20) {
        m_chatHistory.removeFirst();
    }
}

QString AIAssistant::getHistoryFilePath(const QString &questionId) const
{
    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QString historyDir = dataPath + "/ChatHistory";
    
    QDir dir;
    if (!dir.exists(historyDir)) {
        dir.mkpath(historyDir);
    }
    
    return historyDir + "/" + questionId + "_chat.json";
}

QString ChatMessage::toJson() const
{
    QJsonObject obj;
    obj["role"] = role;
    obj["content"] = content;
    obj["timestamp"] = timestamp.toString(Qt::ISODate);
    
    return QString(QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

ChatMessage ChatMessage::fromJson(const QString &json)
{
    ChatMessage msg;
    
    QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
    if (!doc.isObject()) {
        return msg;
    }
    
    QJsonObject obj = doc.object();
    msg.role = obj["role"].toString();
    msg.content = obj["content"].toString();
    msg.timestamp = QDateTime::fromString(obj["timestamp"].toString(), Qt::ISODate);
    
    return msg;
}
