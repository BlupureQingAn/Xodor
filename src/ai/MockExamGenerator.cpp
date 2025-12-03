#include "MockExamGenerator.h"
#include "OllamaClient.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDir>

MockExamGenerator::MockExamGenerator(OllamaClient *aiClient, QObject *parent)
    : QObject(parent)
    , m_aiClient(aiClient)
    , m_currentExamIndex(0)
    , m_totalExams(0)
{
    if (m_aiClient) {
        connect(m_aiClient, &OllamaClient::codeAnalysisReady,
                this, &MockExamGenerator::onAIResponse);
        connect(m_aiClient, &OllamaClient::error,
                this, &MockExamGenerator::onAIError);
    }
}

ExamPattern MockExamGenerator::analyzeQuestionBank(const QVector<Question> &questions, const QString &categoryName)
{
    ExamPattern pattern;
    pattern.categoryName = categoryName;
    
    if (questions.isEmpty()) {
        return pattern;
    }
    
    // 统计难度分布
    QMap<Difficulty, int> diffCount;
    for (const Question &q : questions) {
        diffCount[q.difficulty()]++;
    }
    
    int total = questions.size();
    for (auto it = diffCount.begin(); it != diffCount.end(); ++it) {
        pattern.difficultyRatio[it.key()] = static_cast<double>(it.value()) / total;
    }
    
    // 统计知识点分布
    QMap<QString, int> topicCount;
    for (const Question &q : questions) {
        for (const QString &tag : q.tags()) {
            topicCount[tag]++;
        }
    }
    
    // 取前10个最常见的知识点
    QList<QString> sortedTopics = topicCount.keys();
    std::sort(sortedTopics.begin(), sortedTopics.end(), 
              [&topicCount](const QString &a, const QString &b) {
        return topicCount[a] > topicCount[b];
    });
    
    if (sortedTopics.size() > 10) {
        sortedTopics = sortedTopics.mid(0, 10);
    }
    
    int topicTotal = 0;
    for (const QString &topic : sortedTopics) {
        topicTotal += topicCount[topic];
    }
    
    for (const QString &topic : sortedTopics) {
        pattern.topicRatio[topic] = static_cast<double>(topicCount[topic]) / topicTotal;
    }
    
    // 分析题目数量（假设按套题组织）
    pattern.questionsPerExam = qMin(4, questions.size());
    
    // 默认代码限制
    pattern.supportedLanguages = {"C++", "Python", "Java"};
    
    // 题号规则
    for (int i = 1; i <= pattern.questionsPerExam; ++i) {
        pattern.questionTitlePattern.append(QString("第%1题").arg(i));
    }
    
    return pattern;
}

bool MockExamGenerator::savePattern(const QString &bankPath, const ExamPattern &pattern)
{
    QDir dir(bankPath);
    QString filePath = dir.filePath("出题模式规律.json");
    
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    
    file.write(pattern.toJson().toUtf8());
    file.close();
    return true;
}

ExamPattern MockExamGenerator::loadPattern(const QString &bankPath)
{
    QDir dir(bankPath);
    QString filePath = dir.filePath("出题模式规律.json");
    
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return ExamPattern();
    }
    
    QString json = file.readAll();
    file.close();
    
    return ExamPattern::fromJson(json);
}

void MockExamGenerator::generateMockExam(const ExamPattern &pattern, int examCount)
{
    m_currentPattern = pattern;
    m_totalExams = examCount;
    m_currentExamIndex = 0;
    
    emit progressUpdated(0, QString("开始生成模拟题库..."));
    
    // 生成第一套题
    if (m_aiClient && examCount > 0) {
        QString prompt = buildPrompt(pattern, 1);
        emit progressUpdated(10, QString("正在生成第 1/%1 套题...").arg(examCount));
        m_aiClient->analyzeCode("", prompt);
    }
}

QString MockExamGenerator::buildPrompt(const ExamPattern &pattern, int examIndex)
{
    QString prompt = R"(
你是一个专业的编程竞赛题目生成助手。请基于以下出题规则，生成一套完整的模拟题。

【出题规则】
分类：%1
题目数量：%2 道
时间限制：%3 分钟
单题时间限制：%4 ms
内存限制：%5 MB
支持语言：%6

【难度分布】
%7

【知识点分布】
%8

【生成要求】
1. 题目风格：
   - 符合 %1 竞赛风格
   - 题目描述清晰完整
   - 包含完整的输入输出格式说明
   - 包含约束条件和数据范围

2. 测试数据要求：
   - 每道题生成 5-8 组测试数据
   - 覆盖基础测试、边界条件、异常情况、等价类场景
   - 输入与输出严格对应
   - 每组数据包含描述说明

3. 题目编号：
   - 按照 %9 格式命名

【输出格式】
返回纯 JSON 格式，结构如下：
{
  "examTitle": "模拟题 %10",
  "questions": [
    {
      "title": "第1题：题目标题",
      "difficulty": "简单/中等/困难",
      "description": "完整的题目描述，包括：\n【题目描述】\n问题描述内容\n\n【输入格式】\n输入说明\n\n【输出格式】\n输出说明\n\n【数据范围】\n约束条件\n\n【样例说明】\n样例解释",
      "tags": ["数组", "哈希表"],
      "testCases": [
        {
          "input": "输入数据",
          "output": "期望输出",
          "description": "基本测试"
        },
        {
          "input": "输入数据",
          "output": "期望输出",
          "description": "边界条件：最小值"
        },
        {
          "input": "输入数据",
          "output": "期望输出",
          "description": "边界条件：最大值"
        },
        {
          "input": "输入数据",
          "output": "期望输出",
          "description": "特殊情况：空输入"
        },
        {
          "input": "输入数据",
          "output": "期望输出",
          "description": "等价类测试"
        }
      ],
      "timeLimit": %4,
      "memoryLimit": %5
    }
  ]
}

【注意事项】
- 题目要有创新性，不要直接复制现有题目
- 题目难度要符合难度分布要求
- 测试数据要全面，确保能充分验证代码正确性
- 返回纯 JSON，不要包含其他文字或代码块标记

请开始生成。
)";

    // 构建难度分布说明
    QString diffStr;
    for (auto it = pattern.difficultyRatio.begin(); it != pattern.difficultyRatio.end(); ++it) {
        QString diffName;
        switch (it.key()) {
            case Difficulty::Easy: diffName = "简单"; break;
            case Difficulty::Medium: diffName = "中等"; break;
            case Difficulty::Hard: diffName = "困难"; break;
        }
        diffStr += QString("- %1: %2%\n").arg(diffName).arg(it.value() * 100, 0, 'f', 0);
    }
    
    // 构建知识点分布说明
    QString topicStr;
    for (auto it = pattern.topicRatio.begin(); it != pattern.topicRatio.end(); ++it) {
        topicStr += QString("- %1: %2%\n").arg(it.key()).arg(it.value() * 100, 0, 'f', 0);
    }
    
    // 题号格式
    QString titlePattern = pattern.questionTitlePattern.join("、");
    
    prompt = prompt
        .arg(pattern.categoryName)
        .arg(pattern.questionsPerExam)
        .arg(pattern.timeLimit)
        .arg(pattern.timeLimitPerQuestion)
        .arg(pattern.memoryLimit)
        .arg(pattern.supportedLanguages.join(", "))
        .arg(diffStr)
        .arg(topicStr)
        .arg(titlePattern)
        .arg(examIndex);
    
    return prompt;
}

void MockExamGenerator::onAIResponse(const QString &response)
{
    emit progressUpdated(50 + (m_currentExamIndex * 40 / m_totalExams), 
                        QString("正在解析第 %1/%2 套题...").arg(m_currentExamIndex + 1).arg(m_totalExams));
    
    QVector<Question> questions = parseAIResponse(response, m_currentPattern);
    
    if (!questions.isEmpty()) {
        emit examGenerated(questions, m_currentExamIndex + 1);
        m_currentExamIndex++;
        
        // 继续生成下一套题
        if (m_currentExamIndex < m_totalExams) {
            QString prompt = buildPrompt(m_currentPattern, m_currentExamIndex + 1);
            emit progressUpdated(10 + (m_currentExamIndex * 40 / m_totalExams), 
                               QString("正在生成第 %1/%2 套题...").arg(m_currentExamIndex + 1).arg(m_totalExams));
            m_aiClient->analyzeCode("", prompt);
        } else {
            emit progressUpdated(100, "所有模拟题生成完成！");
            emit generationComplete(m_totalExams);
        }
    } else {
        emit error(QString("第 %1 套题解析失败").arg(m_currentExamIndex + 1));
    }
}

QVector<Question> MockExamGenerator::parseAIResponse(const QString &response, const ExamPattern &pattern)
{
    QVector<Question> questions;
    
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
        return questions;
    }
    
    QJsonObject root = doc.object();
    QJsonArray questionsArray = root["questions"].toArray();
    
    for (const QJsonValue &val : questionsArray) {
        QJsonObject qObj = val.toObject();
        
        Question q;
        q.setId(QString("mock_%1_%2").arg(pattern.categoryName).arg(qHash(qObj["title"].toString())));
        q.setTitle(qObj["title"].toString());
        q.setDescription(qObj["description"].toString());
        
        // 解析难度
        QString diffStr = qObj["difficulty"].toString();
        if (diffStr.contains("简单") || diffStr.contains("easy", Qt::CaseInsensitive)) {
            q.setDifficulty(Difficulty::Easy);
        } else if (diffStr.contains("困难") || diffStr.contains("hard", Qt::CaseInsensitive)) {
            q.setDifficulty(Difficulty::Hard);
        } else {
            q.setDifficulty(Difficulty::Medium);
        }
        
        // 解析标签
        QJsonArray tagsArray = qObj["tags"].toArray();
        QStringList tags;
        for (const QJsonValue &tagVal : tagsArray) {
            tags.append(tagVal.toString());
        }
        q.setTags(tags);
        
        // 解析测试用例
        QJsonArray testCasesArray = qObj["testCases"].toArray();
        QVector<TestCase> testCases;
        for (const QJsonValue &tcVal : testCasesArray) {
            QJsonObject tcObj = tcVal.toObject();
            TestCase tc;
            tc.input = tcObj["input"].toString();
            tc.expectedOutput = tcObj["output"].toString();
            tc.description = tcObj["description"].toString();
            tc.isAIGenerated = true;  // 标记为AI生成
            testCases.append(tc);
        }
        q.setTestCases(testCases);
        q.setType(QuestionType::Code);
        
        questions.append(q);
    }
    
    return questions;
}

void MockExamGenerator::onAIError(const QString &error)
{
    emit this->error(QString("AI生成错误: %1").arg(error));
}

// ExamPattern 序列化
QString ExamPattern::toJson() const
{
    QJsonObject obj;
    obj["categoryName"] = categoryName;
    obj["questionsPerExam"] = questionsPerExam;
    obj["timeLimit"] = timeLimit;
    obj["timeLimitPerQuestion"] = timeLimitPerQuestion;
    obj["memoryLimit"] = memoryLimit;
    obj["supportedLanguages"] = QJsonArray::fromStringList(supportedLanguages);
    obj["questionTitlePattern"] = QJsonArray::fromStringList(questionTitlePattern);
    
    QJsonObject diffObj;
    for (auto it = difficultyRatio.begin(); it != difficultyRatio.end(); ++it) {
        QString key;
        switch (it.key()) {
            case Difficulty::Easy: key = "easy"; break;
            case Difficulty::Medium: key = "medium"; break;
            case Difficulty::Hard: key = "hard"; break;
        }
        diffObj[key] = it.value();
    }
    obj["difficultyRatio"] = diffObj;
    
    QJsonObject topicObj;
    for (auto it = topicRatio.begin(); it != topicRatio.end(); ++it) {
        topicObj[it.key()] = it.value();
    }
    obj["topicRatio"] = topicObj;
    
    return QJsonDocument(obj).toJson(QJsonDocument::Indented);
}

ExamPattern ExamPattern::fromJson(const QString &json)
{
    ExamPattern pattern;
    
    QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
    if (doc.isNull() || !doc.isObject()) {
        return pattern;
    }
    
    QJsonObject obj = doc.object();
    pattern.categoryName = obj["categoryName"].toString();
    pattern.questionsPerExam = obj["questionsPerExam"].toInt(4);
    pattern.timeLimit = obj["timeLimit"].toInt(180);
    pattern.timeLimitPerQuestion = obj["timeLimitPerQuestion"].toInt(1000);
    pattern.memoryLimit = obj["memoryLimit"].toInt(256);
    
    QJsonArray langArray = obj["supportedLanguages"].toArray();
    for (const QJsonValue &val : langArray) {
        pattern.supportedLanguages.append(val.toString());
    }
    
    QJsonArray titleArray = obj["questionTitlePattern"].toArray();
    for (const QJsonValue &val : titleArray) {
        pattern.questionTitlePattern.append(val.toString());
    }
    
    QJsonObject diffObj = obj["difficultyRatio"].toObject();
    for (auto it = diffObj.begin(); it != diffObj.end(); ++it) {
        Difficulty diff;
        if (it.key() == "easy") diff = Difficulty::Easy;
        else if (it.key() == "hard") diff = Difficulty::Hard;
        else diff = Difficulty::Medium;
        pattern.difficultyRatio[diff] = it.value().toDouble();
    }
    
    QJsonObject topicObj = obj["topicRatio"].toObject();
    for (auto it = topicObj.begin(); it != topicObj.end(); ++it) {
        pattern.topicRatio[it.key()] = it.value().toDouble();
    }
    
    return pattern;
}
