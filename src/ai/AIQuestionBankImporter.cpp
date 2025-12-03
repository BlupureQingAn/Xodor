#include "AIQuestionBankImporter.h"
#include "OllamaClient.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDateTime>

AIQuestionBankImporter::AIQuestionBankImporter(OllamaClient *aiClient, QObject *parent)
    : QObject(parent)
    , m_aiClient(aiClient)
    , m_currentStage(ImportStage::Idle)
    , m_cancelled(false)
    , m_currentFileIndex(0)
    , m_currentQuestionIndex(0)
{
    if (m_aiClient) {
        connect(m_aiClient, &OllamaClient::codeAnalysisReady,
                this, &AIQuestionBankImporter::onAIResponse);
        connect(m_aiClient, &OllamaClient::error,
                this, &AIQuestionBankImporter::onAIError);
    }
}

void AIQuestionBankImporter::startImport(const QString &sourcePath, const QString &categoryName)
{
    m_sourcePath = sourcePath;
    m_categoryName = categoryName;
    m_cancelled = false;
    m_currentFileIndex = 0;
    m_currentQuestionIndex = 0;
    m_questions.clear();
    
    // 开始第一阶段：复制文件
    m_currentStage = ImportStage::CopyingFiles;
    emit stageChanged(m_currentStage, "正在复制文件到原始题库...");
    emit progressUpdated(5, "复制文件中...");
    
    processStage1_CopyFiles();
}

void AIQuestionBankImporter::cancelImport()
{
    m_cancelled = true;
    emit importFailed("用户取消导入");
}

// ========== 阶段1：复制文件 ==========
void AIQuestionBankImporter::processStage1_CopyFiles()
{
    if (m_cancelled) return;
    
    // 复制文件到原始题库
    if (!copyToOriginalBank(m_sourcePath, m_categoryName)) {
        emit importFailed("复制文件失败");
        return;
    }
    
    // 获取所有MD文件
    QString originalPath = getOriginalBankPath(m_categoryName);
    QDir dir(originalPath);
    m_mdFiles = dir.entryList(QStringList() << "*.md", QDir::Files, QDir::Name);
    
    if (m_mdFiles.isEmpty()) {
        emit importFailed("未找到Markdown文件");
        return;
    }
    
    emit progressUpdated(10, QString("已复制 %1 个文件").arg(m_mdFiles.size()));
    
    // 进入第二阶段：AI分析格式
    m_currentStage = ImportStage::AnalyzingFormat;
    emit stageChanged(m_currentStage, "AI正在分析题目格式...");
    processStage2_AnalyzeFormat();
}

// ========== 阶段2：AI分析格式 ==========
void AIQuestionBankImporter::processStage2_AnalyzeFormat()
{
    if (m_cancelled) return;
    
    emit progressUpdated(15, "正在抽取样本文件...");
    
    // 获取样本文件（最多3个）
    QStringList sampleFiles = getSampleFiles(getOriginalBankPath(m_categoryName), 3);
    
    if (sampleFiles.isEmpty()) {
        emit importFailed("无法读取样本文件");
        return;
    }
    
    emit progressUpdated(20, QString("已抽取 %1 个样本文件，正在分析...").arg(sampleFiles.size()));
    
    // 构建AI提示词
    QString prompt = buildFormatAnalysisPrompt(sampleFiles);
    
    // 发送给AI分析
    m_aiClient->analyzeCode("", prompt);
}

// ========== 阶段3：生成解析规则 ==========
void AIQuestionBankImporter::processStage3_GenerateRules()
{
    if (m_cancelled) return;
    
    emit progressUpdated(30, "正在生成解析规则...");
    
    // 保存解析规则
    if (!saveParseRule(m_parseRule)) {
        emit importFailed("保存解析规则失败");
        return;
    }
    
    emit progressUpdated(35, "解析规则已生成");
    
    // 进入第四阶段：解析题目
    m_currentStage = ImportStage::ParsingQuestions;
    emit stageChanged(m_currentStage, "正在解析题目...");
    processStage4_ParseQuestions();
}

// ========== 阶段4：解析题目 ==========
void AIQuestionBankImporter::processStage4_ParseQuestions()
{
    if (m_cancelled) return;
    
    if (m_currentFileIndex >= m_mdFiles.size()) {
        // 所有文件解析完成，进入下一阶段
        emit progressUpdated(60, QString("已解析 %1 道题目").arg(m_questions.size()));
        
        m_currentStage = ImportStage::GeneratingTestData;
        emit stageChanged(m_currentStage, "AI正在生成测试数据...");
        processStage5_GenerateTestData();
        return;
    }
    
    // 读取当前文件
    QString filePath = QDir(getOriginalBankPath(m_categoryName)).filePath(m_mdFiles[m_currentFileIndex]);
    QFile file(filePath);
    
    if (!file.open(QIODevice::ReadOnly)) {
        m_currentFileIndex++;
        processStage4_ParseQuestions();
        return;
    }
    
    QString content = QString::fromUtf8(file.readAll());
    file.close();
    
    int progress = 40 + (m_currentFileIndex * 20 / m_mdFiles.size());
    emit progressUpdated(progress, 
        QString("正在解析文件 %1/%2: %3")
        .arg(m_currentFileIndex + 1)
        .arg(m_mdFiles.size())
        .arg(m_mdFiles[m_currentFileIndex]));
    
    // 构建解析提示词
    QString prompt = buildParsePrompt(content, m_parseRule);
    
    // 发送给AI解析
    m_aiClient->analyzeCode("", prompt);
}

// ========== 阶段5：生成测试数据 ==========
void AIQuestionBankImporter::processStage5_GenerateTestData()
{
    if (m_cancelled) return;
    
    if (m_currentQuestionIndex >= m_questions.size()) {
        // 所有测试数据生成完成
        emit progressUpdated(80, "测试数据生成完成");
        
        m_currentStage = ImportStage::OrganizingQuestions;
        emit stageChanged(m_currentStage, "正在组织题目...");
        processStage6_OrganizeQuestions();
        return;
    }
    
    Question &question = m_questions[m_currentQuestionIndex];
    
    // 如果已有测试用例，生成3-5组补充数据
    int existingCount = question.testCases().size();
    int additionalCount = existingCount > 0 ? qMin(5, 7 - existingCount) : 5;
    
    int progress = 65 + (m_currentQuestionIndex * 15 / m_questions.size());
    emit progressUpdated(progress,
        QString("正在为题目 %1/%2 生成测试数据...")
        .arg(m_currentQuestionIndex + 1)
        .arg(m_questions.size()));
    
    // 构建测试数据生成提示词
    QString prompt = buildTestDataPrompt(question, additionalCount);
    
    // 发送给AI生成
    m_aiClient->analyzeCode("", prompt);
}

// ========== 阶段6：组织题目 ==========
void AIQuestionBankImporter::processStage6_OrganizeQuestions()
{
    if (m_cancelled) return;
    
    emit progressUpdated(85, "正在保存题目...");
    
    // 保存题目到基础题库
    if (!saveQuestions(m_questions, m_categoryName)) {
        emit importFailed("保存题目失败");
        return;
    }
    
    emit progressUpdated(90, "题目已保存");
    
    // 进入第七阶段：分析出题规律
    m_currentStage = ImportStage::AnalyzingPattern;
    emit stageChanged(m_currentStage, "正在分析出题规律...");
    processStage7_AnalyzePattern();
}

// ========== 阶段7：分析出题规律 ==========
void AIQuestionBankImporter::processStage7_AnalyzePattern()
{
    if (m_cancelled) return;
    
    emit progressUpdated(95, "正在分析出题模式...");
    
    // 构建规律分析提示词
    QString prompt = buildPatternAnalysisPrompt(m_questions);
    
    // 发送给AI分析
    m_aiClient->analyzeCode("", prompt);
}

// ========== 阶段8：完成 ==========
void AIQuestionBankImporter::processStage8_Complete()
{
    if (m_cancelled) return;
    
    emit progressUpdated(100, "导入完成！");
    
    m_currentStage = ImportStage::Complete;
    emit stageChanged(m_currentStage, "导入完成");
    emit importComplete(m_categoryName, m_questions.size());
}

// ========== AI响应处理 ==========
void AIQuestionBankImporter::onAIResponse(const QString &response)
{
    if (m_cancelled) return;
    
    switch (m_currentStage) {
        case ImportStage::AnalyzingFormat:
            // 解析格式分析结果
            m_parseRule = parseFormatAnalysisResponse(response);
            m_parseRule.category = m_categoryName;
            processStage3_GenerateRules();
            break;
            
        case ImportStage::ParsingQuestions: {
            // 解析题目
            QVector<Question> parsedQuestions = parseQuestionsResponse(response);
            m_questions.append(parsedQuestions);
            m_currentFileIndex++;
            processStage4_ParseQuestions();
            break;
        }
            
        case ImportStage::GeneratingTestData: {
            // 解析测试数据
            QVector<TestCase> testCases = parseTestDataResponse(response);
            if (!testCases.isEmpty() && m_currentQuestionIndex < m_questions.size()) {
                Question &question = m_questions[m_currentQuestionIndex];
                QVector<TestCase> allCases = question.testCases();
                allCases.append(testCases);
                question.setTestCases(allCases);
            }
            m_currentQuestionIndex++;
            processStage5_GenerateTestData();
            break;
        }
            
        case ImportStage::AnalyzingPattern: {
            // 解析出题规律
            QJsonDocument doc = QJsonDocument::fromJson(response.toUtf8());
            if (!doc.isNull() && doc.isObject()) {
                savePattern(m_categoryName, doc.object());
            }
            processStage8_Complete();
            break;
        }
            
        default:
            break;
    }
}

void AIQuestionBankImporter::onAIError(const QString &error)
{
    emit importFailed(QString("AI处理错误: %1").arg(error));
}

// ========== AI提示词构建 ==========
QString AIQuestionBankImporter::buildFormatAnalysisPrompt(const QStringList &sampleFiles)
{
    QString prompt = R"(
你是一个专业的题目格式分析助手。请分析以下Markdown格式的题目文件，识别题目的结构特征。

【样本文件】
%1

【分析任务】
请识别以下内容的模式：
1. 题目标题的标识（如"第X题"、"题目X"、"# 题目"等）
2. 题目描述的标识（如"【题目描述】"、"问题："、"Description:"等）
3. 输入格式的标识（如"【输入】"、"输入格式："、"Input:"等）
4. 输出格式的标识（如"【输出】"、"输出格式："、"Output:"等）
5. 测试用例的标识（如"测试用例X："、"样例输入："、"Example:"等）
6. 代码限制的标识（如"【时间限制】"、"【内存限制】"等）
7. 题目分割的标识（用于识别单文件中的多道题目）

【输出格式】
返回JSON格式：
{
  "titlePatterns": ["第X题", "题目X"],
  "descriptionPatterns": ["【题目描述】", "问题："],
  "inputPatterns": ["【输入】", "输入格式："],
  "outputPatterns": ["【输出】", "输出格式："],
  "testCasePatterns": ["测试用例", "样例输入"],
  "constraintPatterns": ["【时间限制】", "【内存限制】"],
  "splitPatterns": ["---", "第\\d+题"]
}

请开始分析。
)";
    
    return prompt.arg(sampleFiles.join("\n\n==========\n\n"));
}

QString AIQuestionBankImporter::buildParsePrompt(const QString &fileContent, const ParseRule &rule)
{
    QString prompt = R"(
你是一个专业的题目解析助手。请根据以下解析规则，从Markdown文件中提取题目信息。

【解析规则】
题目标题模式：%1
题目描述模式：%2
输入格式模式：%3
输出格式模式：%4
测试用例模式：%5
约束条件模式：%6
题目分割模式：%7

【文件内容】
%8

【解析任务】
1. 识别文件中的所有题目（可能有多道）
2. 提取每道题的：
   - 标题
   - 完整描述（包括题目描述、输入输出格式、数据范围等）
   - 测试用例（输入和对应输出）
   - 代码限制（时间、内存）
   - 难度（根据题目内容推断：简单/中等/困难）
   - 知识点标签（如：数组、字符串、动态规划等）

【输出格式】
返回JSON格式：
{
  "questions": [
    {
      "title": "题目标题",
      "description": "完整的题目描述",
      "difficulty": "简单/中等/困难",
      "tags": ["数组", "哈希表"],
      "testCases": [
        {
          "input": "输入数据",
          "output": "期望输出",
          "description": "基本测试"
        }
      ],
      "timeLimit": 1000,
      "memoryLimit": 256
    }
  ]
}

请开始解析。
)";
    
    return prompt
        .arg(rule.titlePatterns.join(", "))
        .arg(rule.descriptionPatterns.join(", "))
        .arg(rule.inputPatterns.join(", "))
        .arg(rule.outputPatterns.join(", "))
        .arg(rule.testCasePatterns.join(", "))
        .arg(rule.constraintPatterns.join(", "))
        .arg(rule.splitPatterns.join(", "))
        .arg(fileContent);
}

QString AIQuestionBankImporter::buildTestDataPrompt(const Question &question, int additionalCount)
{
    QString prompt = R"(
你是一个专业的测试数据生成助手。请为以下题目生成 %1 组补充测试数据。

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
3. 每组数据包含：
   - input: 输入数据
   - output: 期望输出
   - description: 测试场景描述（如"边界条件：最小值"）
4. 标注为AI生成

【输出格式】
返回JSON格式：
{
  "testCases": [
    {
      "input": "输入数据",
      "output": "期望输出",
      "description": "边界条件：最小值"
    }
  ]
}

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
    
    return prompt
        .arg(additionalCount)
        .arg(question.title())
        .arg(question.description())
        .arg(existingTests);
}

QString AIQuestionBankImporter::buildPatternAnalysisPrompt(const QVector<Question> &questions)
{
    QString prompt = R"(
你是一个专业的出题规律分析助手。请分析以下题库，总结出题模式。

【题库信息】
总题数：%1
题目列表：
%2

【分析任务】
1. 统计难度分布（简单/中等/困难的比例）
2. 统计知识点分布（各知识点出现的频率）
3. 识别套题数量（如果题目按套组织）
4. 识别题号规则（如"第X题"的命名规律）
5. 识别代码限制的通用规则

【输出格式】
返回JSON格式：
{
  "totalQuestions": %1,
  "questionsPerExam": 4,
  "timeLimit": 180,
  "difficultyDistribution": {
    "easy": 0.3,
    "medium": 0.5,
    "hard": 0.2
  },
  "topicDistribution": {
    "数组": 0.25,
    "字符串": 0.20,
    "动态规划": 0.15
  },
  "questionTitlePattern": ["第1题", "第2题", "第3题", "第4题"],
  "timeLimitPerQuestion": 1000,
  "memoryLimit": 256,
  "supportedLanguages": ["C++", "Python", "Java"]
}

请开始分析。
)";
    
    // 构建题目列表
    QString questionList;
    for (int i = 0; i < qMin(20, questions.size()); ++i) {
        const Question &q = questions[i];
        QString diffName;
        switch (q.difficulty()) {
            case Difficulty::Easy: diffName = "简单"; break;
            case Difficulty::Medium: diffName = "中等"; break;
            case Difficulty::Hard: diffName = "困难"; break;
        }
        questionList += QString("%1. %2 [%3] - 标签：%4\n")
            .arg(i + 1)
            .arg(q.title())
            .arg(diffName)
            .arg(q.tags().join(", "));
    }
    
    return prompt
        .arg(questions.size())
        .arg(questionList);
}

// ========== 解析方法 ==========
ParseRule AIQuestionBankImporter::parseFormatAnalysisResponse(const QString &response)
{
    ParseRule rule;
    
    // 提取JSON
    QString jsonStr = response;
    int jsonStart = response.indexOf('{');
    if (jsonStart >= 0) {
        jsonStr = response.mid(jsonStart);
        int jsonEnd = jsonStr.lastIndexOf('}');
        if (jsonEnd > 0) {
            jsonStr = jsonStr.left(jsonEnd + 1);
        }
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());
    if (!doc.isNull() && doc.isObject()) {
        QJsonObject obj = doc.object();
        
        // 解析各种模式
        if (obj.contains("titlePatterns")) {
            QJsonArray arr = obj["titlePatterns"].toArray();
            for (const QJsonValue &val : arr) {
                rule.titlePatterns.append(val.toString());
            }
        }
        
        if (obj.contains("descriptionPatterns")) {
            QJsonArray arr = obj["descriptionPatterns"].toArray();
            for (const QJsonValue &val : arr) {
                rule.descriptionPatterns.append(val.toString());
            }
        }
        
        if (obj.contains("inputPatterns")) {
            QJsonArray arr = obj["inputPatterns"].toArray();
            for (const QJsonValue &val : arr) {
                rule.inputPatterns.append(val.toString());
            }
        }
        
        if (obj.contains("outputPatterns")) {
            QJsonArray arr = obj["outputPatterns"].toArray();
            for (const QJsonValue &val : arr) {
                rule.outputPatterns.append(val.toString());
            }
        }
        
        if (obj.contains("testCasePatterns")) {
            QJsonArray arr = obj["testCasePatterns"].toArray();
            for (const QJsonValue &val : arr) {
                rule.testCasePatterns.append(val.toString());
            }
        }
        
        if (obj.contains("constraintPatterns")) {
            QJsonArray arr = obj["constraintPatterns"].toArray();
            for (const QJsonValue &val : arr) {
                rule.constraintPatterns.append(val.toString());
            }
        }
        
        if (obj.contains("splitPatterns")) {
            QJsonArray arr = obj["splitPatterns"].toArray();
            for (const QJsonValue &val : arr) {
                rule.splitPatterns.append(val.toString());
            }
        }
    }
    
    return rule;
}

QVector<Question> AIQuestionBankImporter::parseQuestionsResponse(const QString &response)
{
    QVector<Question> questions;
    
    // 提取JSON
    QString jsonStr = response;
    int jsonStart = response.indexOf('{');
    if (jsonStart >= 0) {
        jsonStr = response.mid(jsonStart);
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());
    if (!doc.isNull() && doc.isObject()) {
        QJsonObject root = doc.object();
        QJsonArray questionsArray = root["questions"].toArray();
        
        for (const QJsonValue &val : questionsArray) {
            QJsonObject qObj = val.toObject();
            
            Question q;
            q.setId(QString("q_%1").arg(QDateTime::currentMSecsSinceEpoch()));
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
                tc.isAIGenerated = false;  // 原始数据
                testCases.append(tc);
            }
            q.setTestCases(testCases);
            q.setType(QuestionType::Code);
            
            questions.append(q);
        }
    }
    
    return questions;
}

QVector<TestCase> AIQuestionBankImporter::parseTestDataResponse(const QString &response)
{
    QVector<TestCase> testCases;
    
    // 提取JSON
    QString jsonStr = response;
    int jsonStart = response.indexOf('{');
    if (jsonStart >= 0) {
        jsonStr = response.mid(jsonStart);
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());
    if (!doc.isNull() && doc.isObject()) {
        QJsonObject root = doc.object();
        QJsonArray casesArray = root["testCases"].toArray();
        
        for (const QJsonValue &val : casesArray) {
            QJsonObject caseObj = val.toObject();
            TestCase tc;
            tc.input = caseObj["input"].toString();
            tc.expectedOutput = caseObj["output"].toString();
            tc.description = caseObj["description"].toString();
            tc.isAIGenerated = true;  // AI生成的数据
            testCases.append(tc);
        }
    }
    
    return testCases;
}

// ========== 文件操作 ==========
bool AIQuestionBankImporter::copyToOriginalBank(const QString &sourcePath, const QString &categoryName)
{
    QString destPath = getOriginalBankPath(categoryName);
    
    QDir destDir;
    if (!destDir.mkpath(destPath)) {
        return false;
    }
    
    // 递归复制文件
    QDir sourceDir(sourcePath);
    QFileInfoList entries = sourceDir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
    
    for (const QFileInfo &entry : entries) {
        QString destFilePath = destPath + "/" + entry.fileName();
        
        if (entry.isDir()) {
            // 递归复制目录
            if (!copyToOriginalBank(entry.filePath(), categoryName + "/" + entry.fileName())) {
                return false;
            }
        } else {
            // 复制文件
            QFile::copy(entry.filePath(), destFilePath);
        }
    }
    
    return true;
}

QStringList AIQuestionBankImporter::getSampleFiles(const QString &path, int count)
{
    QStringList samples;
    
    QDir dir(path);
    QStringList mdFiles = dir.entryList(QStringList() << "*.md", QDir::Files, QDir::Name);
    
    int sampleCount = qMin(count, mdFiles.size());
    for (int i = 0; i < sampleCount; ++i) {
        QString filePath = dir.filePath(mdFiles[i]);
        QFile file(filePath);
        
        if (file.open(QIODevice::ReadOnly)) {
            QString content = QString::fromUtf8(file.readAll());
            samples.append(QString("【文件：%1】\n%2").arg(mdFiles[i]).arg(content));
            file.close();
        }
    }
    
    return samples;
}

bool AIQuestionBankImporter::saveParseRule(const ParseRule &rule)
{
    QString configPath = getConfigPath(rule.category);
    
    QDir dir;
    dir.mkpath(QFileInfo(configPath).path());
    
    QFile file(configPath);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    
    file.write(rule.toJson().toUtf8());
    file.close();
    return true;
}

bool AIQuestionBankImporter::saveQuestions(const QVector<Question> &questions, const QString &categoryName)
{
    QString basePath = getBaseBankPath(categoryName);
    
    QDir dir;
    if (!dir.mkpath(basePath)) {
        return false;
    }
    
    // 按文件组织题目（每个文件的题目放在同名文件夹中）
    QMap<QString, QVector<Question>> fileGroups;
    
    for (const Question &q : questions) {
        // 简单起见，所有题目放在一个文件夹
        fileGroups["all"].append(q);
    }
    
    // 保存题目
    for (auto it = fileGroups.begin(); it != fileGroups.end(); ++it) {
        QString folderPath = basePath + "/" + it.key();
        dir.mkpath(folderPath);
        
        for (int i = 0; i < it.value().size(); ++i) {
            const Question &q = it.value()[i];
            QString fileName = QString("第%1题.md").arg(i + 1);
            QString filePath = folderPath + "/" + fileName;
            
            QFile file(filePath);
            if (file.open(QIODevice::WriteOnly)) {
                QString content = QString("# %1\n\n%2\n\n## 测试用例\n\n")
                    .arg(q.title())
                    .arg(q.description());
                
                for (int j = 0; j < q.testCases().size(); ++j) {
                    const TestCase &tc = q.testCases()[j];
                    QString aiTag = tc.isAIGenerated ? " [AI补充]" : "";
                    content += QString("### 测试 %1：%2%3\n\n输入：\n```\n%4\n```\n\n输出：\n```\n%5\n```\n\n")
                        .arg(j + 1)
                        .arg(tc.description)
                        .arg(aiTag)
                        .arg(tc.input)
                        .arg(tc.expectedOutput);
                }
                
                file.write(content.toUtf8());
                file.close();
            }
        }
    }
    
    return true;
}

bool AIQuestionBankImporter::savePattern(const QString &categoryName, const QJsonObject &pattern)
{
    QString patternPath = getBaseBankPath(categoryName) + "/出题模式规律.json";
    
    QFile file(patternPath);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    
    file.write(QJsonDocument(pattern).toJson(QJsonDocument::Indented));
    file.close();
    return true;
}

// ========== 路径管理 ==========
QString AIQuestionBankImporter::getOriginalBankPath(const QString &category) const
{
    return QString("原始题库/%1").arg(category);
}

QString AIQuestionBankImporter::getBaseBankPath(const QString &category) const
{
    return QString("基础题库/%1").arg(category);
}

QString AIQuestionBankImporter::getConfigPath(const QString &category) const
{
    return QString("config/%1_parse_rule.json").arg(category);
}

// ========== ParseRule 序列化 ==========
QJsonObject ParseRule::toJson() const
{
    QJsonObject obj;
    obj["category"] = category;
    obj["titlePatterns"] = QJsonArray::fromStringList(titlePatterns);
    obj["descriptionPatterns"] = QJsonArray::fromStringList(descriptionPatterns);
    obj["inputPatterns"] = QJsonArray::fromStringList(inputPatterns);
    obj["outputPatterns"] = QJsonArray::fromStringList(outputPatterns);
    obj["testCasePatterns"] = QJsonArray::fromStringList(testCasePatterns);
    obj["constraintPatterns"] = QJsonArray::fromStringList(constraintPatterns);
    obj["splitPatterns"] = QJsonArray::fromStringList(splitPatterns);
    return obj;
}

ParseRule ParseRule::fromJson(const QJsonObject &json)
{
    ParseRule rule;
    rule.category = json["category"].toString();
    
    auto parseArray = [](const QJsonArray &arr) {
        QStringList list;
        for (const QJsonValue &val : arr) {
            list.append(val.toString());
        }
        return list;
    };
    
    rule.titlePatterns = parseArray(json["titlePatterns"].toArray());
    rule.descriptionPatterns = parseArray(json["descriptionPatterns"].toArray());
    rule.inputPatterns = parseArray(json["inputPatterns"].toArray());
    rule.outputPatterns = parseArray(json["outputPatterns"].toArray());
    rule.testCasePatterns = parseArray(json["testCasePatterns"].toArray());
    rule.constraintPatterns = parseArray(json["constraintPatterns"].toArray());
    rule.splitPatterns = parseArray(json["splitPatterns"].toArray());
    
    return rule;
}
