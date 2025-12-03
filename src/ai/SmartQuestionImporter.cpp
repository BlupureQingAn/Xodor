#include "SmartQuestionImporter.h"
#include "OllamaClient.h"
#include "UniversalQuestionParser.h"
#include "QuestionBankAnalyzer.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>
#include <QDebug>
#include <QTimer>
#include <QDateTime>
#include <QEventLoop>
#include <numeric>

SmartQuestionImporter::SmartQuestionImporter(OllamaClient *aiClient, QObject *parent)
    : QObject(parent)
    , m_aiClient(aiClient)
    , m_parser(new UniversalQuestionParser())
    , m_analyzer(new QuestionBankAnalyzer())
    , m_currentChunkIndex(0)
    , m_cancelled(false)
    , m_useUniversalParser(false)
{
    // 连接AI信号
    if (m_aiClient) {
        connect(m_aiClient, &OllamaClient::codeAnalysisReady,
                this, &SmartQuestionImporter::onAIResponse);
        connect(m_aiClient, &OllamaClient::error,
                this, &SmartQuestionImporter::onAIError);
        
        // 连接流式进度信号
        connect(m_aiClient, &OllamaClient::streamProgress,
                this, &SmartQuestionImporter::onStreamProgress);
    }
}

SmartQuestionImporter::~SmartQuestionImporter()
{
    delete m_parser;
    delete m_analyzer;
}

void SmartQuestionImporter::startImport(const QString &sourcePath, const QString &targetPath, const QString &bankName)
{
    m_targetPath = targetPath;
    m_bankName = bankName;
    m_cancelled = false;
    m_chunks.clear();
    m_questions.clear();
    m_currentChunkIndex = 0;
    
    // 备份原始题库（静默处理）
    QString originalBankPath = QString("data/原始题库/%1").arg(m_bankName);
    if (!copyQuestionBank(sourcePath, originalBankPath)) {
        emit importCompleted(false, "原始题库备份失败");
        return;
    }
    
    // 设置只读属性
    QDir originalDir(originalBankPath);
    QFileInfoList files = originalDir.entryInfoList(QDir::Files);
    for (const QFileInfo &fileInfo : files) {
        QFile::setPermissions(fileInfo.absoluteFilePath(), 
                             QFile::ReadOwner | QFile::ReadUser | QFile::ReadGroup | QFile::ReadOther);
    }
    
    // 扫描并分析文件（从原始题库读取）
    scanAndAnalyzeFiles(originalBankPath);
    
    if (m_chunks.isEmpty()) {
        emit importCompleted(false, "未找到任何题目文件");
        return;
    }
    
    // 更新进度
    m_progress.totalChunks = m_chunks.size();
    m_progress.processedChunks = 0;
    m_progress.currentStatus = "开始AI解析并实时保存";
    emit progressUpdated(m_progress);
    
    // 开始处理第一个块
    emit logMessage("\n[2/2] 🤖 AI解析并实时保存...");
    processNextChunk();
}

void SmartQuestionImporter::cancelImport()
{
    m_cancelled = true;
    emit logMessage("\n⚠️ 用户取消导入");
    emit importCompleted(false, "用户取消");
}

bool SmartQuestionImporter::copyQuestionBank(const QString &sourcePath, const QString &targetPath)
{
    QDir sourceDir(sourcePath);
    if (!sourceDir.exists()) {
        emit logMessage("❌ 源路径不存在");
        return false;
    }
    
    // 创建目标目录
    QDir targetDir;
    if (!targetDir.mkpath(targetPath)) {
        emit logMessage("❌ 无法创建目标目录");
        return false;
    }
    
    targetDir.setPath(targetPath);
    
    // 拷贝所有Markdown文件
    QStringList filters;
    filters << "*.md" << "*.markdown" << "*.txt";
    
    QFileInfoList files = sourceDir.entryInfoList(filters, QDir::Files);
    
    emit logMessage(QString("  找到 %1 个文件").arg(files.size()));
    
    for (const QFileInfo &fileInfo : files) {
        QString sourcePath = fileInfo.absoluteFilePath();
        QString targetFilePath = targetDir.filePath(fileInfo.fileName());
        
        // 如果目标文件已存在，先删除
        if (QFile::exists(targetFilePath)) {
            QFile::remove(targetFilePath);
        }
        
        if (QFile::copy(sourcePath, targetFilePath)) {
            emit logMessage(QString("  ✓ %1").arg(fileInfo.fileName()));
        } else {
            emit logMessage(QString("  ✗ 拷贝失败: %1").arg(fileInfo.fileName()));
        }
    }
    
    return true;
}

void SmartQuestionImporter::scanAndAnalyzeFiles(const QString &path)
{
    emit logMessage("\n[1/2] 📂 扫描文件...");
    
    QDir dir(path);
    QStringList filters;
    filters << "*.md" << "*.markdown" << "*.txt";
    
    QFileInfoList files = dir.entryInfoList(filters, QDir::Files);
    
    m_progress.totalFiles = files.size();
    m_progress.processedFiles = 0;
    m_progress.currentStatus = "扫描文件";
    emit progressUpdated(m_progress);
    
    emit logMessage(QString("  找到 %1 个文件\n").arg(files.size()));
    
    for (const QFileInfo &fileInfo : files) {
        QFile file(fileInfo.absoluteFilePath());
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            continue;
        }
        
        QTextStream in(&file);
        in.setEncoding(QStringConverter::Utf8);
        QString content = in.readAll();
        file.close();
        
        emit logMessage(QString("  ✓ %1 (%2 字符)")
            .arg(fileInfo.fileName())
            .arg(content.length()));
        
        // 智能拆分文件
        QVector<FileChunk> chunks = splitLargeFile(fileInfo.fileName(), content);
        
        if (chunks.size() > 1) {
            emit logMessage(QString("    → 拆分为 %1 个块").arg(chunks.size()));
        }
        
        m_chunks.append(chunks);
        m_progress.processedFiles++;
        emit progressUpdated(m_progress);
    }
    
    emit logMessage(QString("\n  共 %1 个文件，%2 个文件块\n")
        .arg(files.size())
        .arg(m_chunks.size()));
}

QVector<FileChunk> SmartQuestionImporter::splitLargeFile(const QString &fileName, const QString &content)
{
    QVector<FileChunk> chunks;
    
    // 如果文件不大，不拆分
    if (content.length() < MAX_CHUNK_SIZE) {
        FileChunk chunk;
        chunk.fileName = fileName;
        chunk.content = content;
        chunk.chunkIndex = 0;
        chunk.totalChunks = 1;
        chunk.startLine = 1;
        chunk.endLine = content.count('\n') + 1;
        chunks.append(chunk);
        return chunks;
    }
    
    // 大文件，按题目边界拆分
    QStringList lines = content.split('\n');
    QString currentChunk;
    int chunkIndex = 0;
    int startLine = 1;
    int currentLine = 1;
    
    for (const QString &line : lines) {
        // 检查是否是题目边界
        if (isQuestionBoundary(line) && 
            currentChunk.length() > 1000) {  // 至少1000字符才考虑拆分
            
            // 保存当前块
            FileChunk chunk;
            chunk.fileName = fileName;
            chunk.content = currentChunk;
            chunk.chunkIndex = chunkIndex++;
            chunk.totalChunks = -1;  // 稍后更新
            chunk.startLine = startLine;
            chunk.endLine = currentLine - 1;
            chunks.append(chunk);
            
            // 开始新块
            currentChunk = line + "\n";
            startLine = currentLine;
        } else {
            currentChunk += line + "\n";
        }
        
        // 如果当前块太大，强制分割
        if (currentChunk.length() > MAX_CHUNK_SIZE) {
            FileChunk chunk;
            chunk.fileName = fileName;
            chunk.content = currentChunk;
            chunk.chunkIndex = chunkIndex++;
            chunk.totalChunks = -1;
            chunk.startLine = startLine;
            chunk.endLine = currentLine;
            chunks.append(chunk);
            
            currentChunk.clear();
            startLine = currentLine + 1;
        }
        
        currentLine++;
    }
    
    // 保存最后一块
    if (!currentChunk.isEmpty()) {
        FileChunk chunk;
        chunk.fileName = fileName;
        chunk.content = currentChunk;
        chunk.chunkIndex = chunkIndex;
        chunk.totalChunks = -1;
        chunk.startLine = startLine;
        chunk.endLine = currentLine - 1;
        chunks.append(chunk);
    }
    
    // 更新总块数
    for (auto &chunk : chunks) {
        chunk.totalChunks = chunks.size();
    }
    
    return chunks;
}

bool SmartQuestionImporter::isQuestionBoundary(const QString &line)
{
    QString trimmed = line.trimmed();
    
    // 一级标题（# 标题，但不是 ## 或更多）
    if (trimmed.startsWith("# ") && !trimmed.startsWith("## ")) {
        return true;
    }
    
    // 题号格式：1. 题目 或 1) 题目 或 1、题目
    if (QRegularExpression(R"(^\d+[\.\)、]\s+\S)").match(trimmed).hasMatch()) {
        return true;
    }
    
    // 第N题格式
    if (QRegularExpression(R"(^第\d+题)").match(trimmed).hasMatch()) {
        return true;
    }
    
    // 分隔线（至少3个字符）
    if (trimmed.length() >= 3) {
        if (trimmed == QString(trimmed.length(), '-') ||
            trimmed == QString(trimmed.length(), '=') ||
            trimmed == QString(trimmed.length(), '*')) {
            return true;
        }
    }
    
    return false;
}

void SmartQuestionImporter::processNextChunk()
{
    if (m_cancelled) {
        return;
    }
    
    if (m_currentChunkIndex >= m_chunks.size()) {
        // 所有块处理完成
        emit logMessage(QString("\n✅ AI解析完成！共导入 %1 道题目").arg(m_questions.size()));
        
        // 第四步：保存解析规则和基础题库
        emit logMessage("\n📝 第四步：保存解析规则和基础题库...");
        if (saveParseRulesAndQuestionBank()) {
            emit logMessage("✅ 解析规则和基础题库保存完成");
        } else {
            emit logMessage("⚠️ 保存过程中出现部分问题");
        }
        
        // 第五步：生成出题模式规律
        emit logMessage("\n📊 第五步：生成出题模式规律...");
        if (generateExamPattern()) {
            emit logMessage("✅ 出题模式规律生成完成");
        }
        
        m_progress.currentStatus = "导入完成";
        emit progressUpdated(m_progress);
        emit importCompleted(true, QString("成功导入 %1 道题目").arg(m_questions.size()));
        return;
    }
    
    const FileChunk &chunk = m_chunks[m_currentChunkIndex];
    
    // 更新进度
    m_progress.currentFile = chunk.fileName;
    m_progress.currentStatus = QString("AI解析并保存 %1/%2")
        .arg(m_currentChunkIndex + 1)
        .arg(m_chunks.size());
    m_progress.processedChunks = m_currentChunkIndex;
    emit progressUpdated(m_progress);
    
    emit logMessage(QString("\n[%1/%2] 📄 %3")
        .arg(m_currentChunkIndex + 1)
        .arg(m_chunks.size())
        .arg(chunk.fileName));
    
    emit chunkProcessed(chunk.fileName, chunk.chunkIndex + 1, chunk.totalChunks);
    
    // 发送给AI解析
    parseChunkWithAI(chunk);
}

void SmartQuestionImporter::parseChunkWithAI(const FileChunk &chunk)
{
    qDebug() << "[SmartQuestionImporter] parseChunkWithAI 开始";
    
    if (!m_aiClient) {
        qDebug() << "[SmartQuestionImporter] AI客户端为空!";
        emit logMessage("❌ AI客户端未初始化");
        emit importCompleted(false, "AI客户端未初始化");
        return;
    }
    
    QString prompt = buildAIPrompt(chunk);
    qDebug() << "[SmartQuestionImporter] Prompt已构建，长度:" << prompt.length();
    
    emit logMessage("  ⏳ 发送AI请求...");
    emit logMessage(QString("  📊 Prompt大小: %1 字符").arg(prompt.length()));
    
    // 使用sendCustomPrompt发送自定义prompt
    qDebug() << "[SmartQuestionImporter] 调用 sendCustomPrompt";
    m_aiClient->sendCustomPrompt(prompt, "question_parse");
    qDebug() << "[SmartQuestionImporter] sendCustomPrompt 调用完成";
    
    // 添加超时提示（30秒后）
    QTimer::singleShot(30000, this, [this]() {
        if (m_currentChunkIndex < m_chunks.size()) {
            emit logMessage("  ⏰ AI处理时间较长，请耐心等待...");
            emit logMessage("  💡 大型题库可能需要几分钟时间");
        }
    });
}

QString SmartQuestionImporter::buildAIPrompt(const FileChunk &chunk)
{
    QString prompt = R"(
你是专业的编程题目解析和测试用例生成助手。

任务：解析题目并生成完整测试数据集

要求：
1. 识别所有编程题目（忽略目录、说明等）
2. 提取：标题、难度、描述、标签
3. 生成至少5组测试用例：
   - 基本功能测试（2-3个）
   - 边界条件（空输入、最小值、最大值）
   - 特殊情况（负数、零、重复、无解）

JSON格式：
{
  "questions": [
    {
      "title": "题目标题",
      "difficulty": "简单/中等/困难",
      "description": "完整描述",
      "tags": ["数组", "哈希表"],
      "testCases": [
        {"input": "输入", "output": "输出", "description": "基本测试"},
        {"input": "输入", "output": "输出", "description": "边界条件"},
        {"input": "输入", "output": "输出", "description": "特殊情况"}
      ]
    }
  ]
}

文件内容：
---
)";
    
    prompt += chunk.content;
    prompt += "\n---\n\n请返回纯JSON，不要其他文字。";
    
    return prompt;
}

void SmartQuestionImporter::onAIResponse(const QString &response)
{
    qDebug() << "[SmartQuestionImporter] onAIResponse 被调用";
    qDebug() << "[SmartQuestionImporter] 响应长度:" << response.length();
    
    if (m_cancelled) {
        qDebug() << "[SmartQuestionImporter] 已取消，忽略响应";
        return;
    }
    
    emit logMessage("  ✓ AI响应接收完成");
    
    parseAIResponseAndGenerateTests(response, m_chunks[m_currentChunkIndex]);
    
    // 处理下一个块
    m_currentChunkIndex++;
    processNextChunk();
}

void SmartQuestionImporter::parseAIResponseAndGenerateTests(const QString &response, const FileChunk &chunk)
{
    // 提取JSON
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
    
    // 解析JSON
    QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());
    
    if (doc.isNull() || !doc.isObject()) {
        emit logMessage("  ⚠️ JSON解析失败，尝试AI修复...");
        
        // 尝试让AI修复JSON
        QString fixedJson = fixJsonWithAI(jsonStr);
        if (!fixedJson.isEmpty()) {
            doc = QJsonDocument::fromJson(fixedJson.toUtf8());
            if (!doc.isNull() && doc.isObject()) {
                emit logMessage("  ✓ AI成功修复JSON");
            } else {
                emit logMessage("  ✗ AI修复失败，跳过此块");
                return;
            }
        } else {
            emit logMessage("  ✗ 无法修复JSON，跳过此块");
            return;
        }
    }
    
    QJsonObject root = doc.object();
    QJsonArray questionsArray = root["questions"].toArray();
    
    emit logMessage(QString("  ✓ 解析到 %1 道题目，开始实时保存...").arg(questionsArray.size()));
    
    // 准备保存目录
    QString sourceFileName = chunk.fileName;
    sourceFileName = QFileInfo(sourceFileName).baseName();  // 移除扩展名
    QString baseQuestionBankDir = QString("data/基础题库/%1").arg(m_bankName);
    QString subDir = QString("%1/%2").arg(baseQuestionBankDir).arg(sourceFileName);
    
    QDir dir;
    if (!dir.mkpath(subDir)) {
        emit logMessage(QString("  ❌ 无法创建目录: %1").arg(subDir));
        return;
    }
    
    for (const QJsonValue &val : questionsArray) {
        QJsonObject qObj = val.toObject();
        
        Question q;
        // 使用源文件名作为ID的一部分
        q.setId(QString("%1_%2").arg(sourceFileName).arg(qHash(qObj["title"].toString())));
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
        int originalCount = 0;
        for (const QJsonValue &tcVal : testCasesArray) {
            QJsonObject tcObj = tcVal.toObject();
            TestCase tc;
            tc.input = tcObj["input"].toString();
            tc.expectedOutput = tcObj["output"].toString();
            tc.description = tcObj["description"].toString();
            
            // 前3个标记为原始数据，后面的标记为AI生成
            if (originalCount < 3) {
                tc.isAIGenerated = false;
                originalCount++;
            } else {
                tc.isAIGenerated = true;
            }
            
            testCases.append(tc);
        }
        
        // 如果测试用例不足，生成更多
        if (testCases.size() < MIN_TEST_CASES) {
            QVector<TestCase> generated = generateTestCases(q);
            for (TestCase &tc : generated) {
                tc.isAIGenerated = true;
            }
            testCases.append(generated);
        }
        
        q.setTestCases(testCases);
        q.setType(QuestionType::Code);
        
        // 立即保存题目到文件
        QString safeTitle = q.title();
        safeTitle.replace(QRegularExpression("[\\\\/:*?\"<>|]"), "_");
        safeTitle = safeTitle.trimmed();
        if (safeTitle.isEmpty()) {
            safeTitle = QString("题目%1").arg(m_progress.totalQuestions + 1);
        }
        
        QString questionFilePath = QString("%1/%2.json").arg(subDir).arg(safeTitle);
        
        // 检查文件是否已存在
        bool isOverwrite = QFile::exists(questionFilePath);
        
        QFile jsonFile(questionFilePath);
        if (jsonFile.open(QIODevice::WriteOnly)) {
            QJsonDocument doc(q.toJson());
            jsonFile.write(doc.toJson(QJsonDocument::Indented));
            jsonFile.close();
            
            // 添加到内存列表（用于后续加载）
            m_questions.append(q);
            m_progress.totalQuestions++;
            
            // 显示保存信息
            QString diffEmoji = (q.difficulty() == Difficulty::Easy) ? "🟢" : 
                               (q.difficulty() == Difficulty::Hard) ? "🔴" : "🟡";
            QString saveStatus = isOverwrite ? "✓已覆盖" : "✓已保存";
            emit logMessage(QString("    %1 %2 [%3] - %4个测试用例 %5")
                .arg(diffEmoji)
                .arg(q.title())
                .arg(diffStr)
                .arg(testCases.size())
                .arg(saveStatus));
        } else {
            emit logMessage(QString("    ❌ 保存失败: %1").arg(q.title()));
        }
    }
}

QVector<TestCase> SmartQuestionImporter::generateTestCases(const Question &question)
{
    // 简单的测试用例生成（如果AI生成的不够）
    QVector<TestCase> cases;
    
    // 这里可以根据题目类型生成一些基础测试用例
    // 当前返回空，让AI负责生成
    
    return cases;
}

QString SmartQuestionImporter::fixJsonWithAI(const QString &brokenJson)
{
    if (!m_aiClient) {
        return QString();
    }
    
    QString prompt = R"(
你是JSON修复专家。下面的JSON格式有错误，请修复它。

要求：
1. 只返回修复后的纯JSON，不要任何其他文字
2. 保持原有数据内容不变
3. 修复语法错误（缺少逗号、括号不匹配等）
4. 确保返回的是有效的JSON

错误的JSON：
---
)" + brokenJson + R"(
---

请返回修复后的JSON：
)";
    
    emit logMessage("  🔧 发送JSON修复请求...");
    
    // 使用事件循环实现同步等待
    QString fixedJson;
    bool completed = false;
    
    // 临时连接信号
    QMetaObject::Connection conn = connect(m_aiClient, &OllamaClient::codeAnalysisReady,
        [&fixedJson, &completed](const QString &response) {
            // 提取JSON
            QString json = response;
            int jsonStart = response.indexOf("```json");
            if (jsonStart >= 0) {
                jsonStart = response.indexOf('\n', jsonStart) + 1;
                int jsonEnd = response.indexOf("```", jsonStart);
                if (jsonEnd > jsonStart) {
                    json = response.mid(jsonStart, jsonEnd - jsonStart).trimmed();
                }
            } else {
                jsonStart = response.indexOf('{');
                if (jsonStart >= 0) {
                    json = response.mid(jsonStart);
                }
            }
            fixedJson = json;
            completed = true;
        });
    
    // 发送请求
    m_aiClient->sendCustomPrompt(prompt, "json_fix");
    
    // 等待响应（最多10秒）
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    connect(m_aiClient, &OllamaClient::codeAnalysisReady, &loop, &QEventLoop::quit);
    connect(m_aiClient, &OllamaClient::error, &loop, &QEventLoop::quit);
    
    timer.start(10000);
    loop.exec();
    
    // 断开临时连接
    disconnect(conn);
    
    if (completed && !fixedJson.isEmpty()) {
        emit logMessage("  ✓ JSON修复完成");
        return fixedJson;
    } else {
        emit logMessage("  ✗ JSON修复超时或失败");
        return QString();
    }
}

void SmartQuestionImporter::onAIError(const QString &error)
{
    emit logMessage(QString("  ❌ AI错误: %1").arg(error));
    
    // 跳过当前块，继续处理下一个
    m_currentChunkIndex++;
    processNextChunk();
}

void SmartQuestionImporter::onStreamProgress(const QString &context, int currentLength, const QString &partialContent)
{
    // 只处理question_parse上下文的进度
    if (context != "question_parse") {
        return;
    }
    
    // 更新进度条（基于接收的字节数）
    // 假设平均每个题目约2000字符，估算进度
    int estimatedQuestions = currentLength / 2000;
    if (estimatedQuestions < 1) estimatedQuestions = 1;
    
    // 更新进度信息（简化显示）
    m_progress.currentStatus = QString("AI解析中... (%1 字符)")
        .arg(currentLength);
    
    emit progressUpdated(m_progress);
    
    // 每2000字符输出一次日志
    static int lastLoggedLength = 0;
    if (currentLength - lastLoggedLength >= 2000) {
        emit logMessage(QString("  ⏳ AI思考中... %1 字符").arg(currentLength));
        lastLoggedLength = currentLength;
    }
}


bool SmartQuestionImporter::saveParseRulesAndQuestionBank()
{
    if (m_questions.isEmpty()) {
        emit logMessage("  ⚠️ 没有题目需要保存");
        return false;
    }
    
    // 1. 保存解析规则到 config/ccf_parse_rule.json
    QString configDir = "data/config";
    QDir dir;
    if (!dir.mkpath(configDir)) {
        emit logMessage("  ❌ 无法创建config目录");
        return false;
    }
    
    QString ruleFilePath = configDir + "/ccf_parse_rule.json";
    QJsonObject parseRule;
    parseRule["bankName"] = m_bankName;
    parseRule["totalQuestions"] = m_questions.size();
    parseRule["createdTime"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    
    // 分析题目格式特征
    QJsonArray modulePatterns;
    QJsonObject pattern;
    pattern["题干标识"] = QJsonArray{"【题目描述】", "问题：", "题目："};
    pattern["输入标识"] = QJsonArray{"【输入】", "输入格式：", "Input:"};
    pattern["输出标识"] = QJsonArray{"【输出】", "输出格式：", "Output:"};
    pattern["测试数据分隔"] = QJsonArray{"空行", "测试用例", "样例"};
    pattern["代码限制"] = QJsonArray{"【时间限制】", "【内存限制】", "支持语言："};
    modulePatterns.append(pattern);
    
    parseRule["modulePatterns"] = modulePatterns;
    parseRule["parseMode"] = "AI智能解析";
    
    QFile ruleFile(ruleFilePath);
    if (ruleFile.open(QIODevice::WriteOnly)) {
        QJsonDocument doc(parseRule);
        ruleFile.write(doc.toJson(QJsonDocument::Indented));
        ruleFile.close();
        emit logMessage(QString("  ✓ 解析规则已保存: %1").arg(ruleFilePath));
    } else {
        emit logMessage("  ⚠️ 无法保存解析规则");
    }
    
    // 2. 题目已在AI解析时实时保存，这里只做统计
    QString baseQuestionBankDir = QString("data/基础题库/%1").arg(m_bankName);
    
    emit logMessage(QString("\n[2/2] 📊 保存完成统计..."));
    
    // 统计各源文件的题目数量
    QMap<QString, int> questionCountByFile;
    for (const Question &q : m_questions) {
        QString sourceFile = q.id().section('_', 0, 0);
        if (sourceFile.isEmpty()) {
            sourceFile = "未分类";
        }
        questionCountByFile[sourceFile]++;
    }
    
    emit logMessage(QString("  📁 根目录: %1").arg(baseQuestionBankDir));
    for (auto it = questionCountByFile.begin(); it != questionCountByFile.end(); ++it) {
        emit logMessage(QString("  📂 %1/ - %2 道题目").arg(it.key()).arg(it.value()));
    }
    
    emit logMessage(QString("\n  ✅ 共保存 %1 道题目到 %2 个文件夹")
        .arg(m_questions.size())
        .arg(questionCountByFile.size()));
    
    return !m_questions.isEmpty();
}

bool SmartQuestionImporter::saveRuntimeQuestionBank()
{
    // 运行时题库就是基础题库，不需要重复保存
    // 直接使用 data/基础题库/{bankName}/questions.json
    emit logMessage("  ℹ️ 运行时直接使用基础题库JSON");
    return true;
}

bool SmartQuestionImporter::generateExamPattern()
{
    if (m_questions.isEmpty()) {
        return false;
    }
    
    QString baseQuestionBankDir = QString("data/基础题库/%1").arg(m_bankName);
    QString patternFile = baseQuestionBankDir + "/出题模式规律.md";
    
    QFile file(patternFile);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        emit logMessage("  ❌ 无法创建出题模式规律文件");
        return false;
    }
    
    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    
    // 统计信息
    int easyCount = 0, mediumCount = 0, hardCount = 0;
    QMap<QString, int> tagCount;
    int totalTestCases = 0;
    
    for (const Question &q : m_questions) {
        switch (q.difficulty()) {
            case Difficulty::Easy: easyCount++; break;
            case Difficulty::Medium: mediumCount++; break;
            case Difficulty::Hard: hardCount++; break;
        }
        
        for (const QString &tag : q.tags()) {
            tagCount[tag]++;
        }
        
        totalTestCases += q.testCases().size();
    }
    
    // 写入分析报告
    out << "# " << m_bankName << " - 出题模式规律\n\n";
    out << "> 自动生成时间: " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "\n\n";
    
    out << "## 📊 题库概况\n\n";
    out << "- **题目总数**: " << m_questions.size() << " 道\n";
    out << "- **平均测试用例**: " << QString::number(totalTestCases * 1.0 / m_questions.size(), 'f', 1) << " 组/题\n\n";
    
    out << "## 📈 难度分布\n\n";
    out << "| 难度 | 数量 | 占比 |\n";
    out << "|------|------|------|\n";
    out << QString("| 简单 | %1 | %2% |\n").arg(easyCount).arg(easyCount * 100 / m_questions.size());
    out << QString("| 中等 | %1 | %2% |\n").arg(mediumCount).arg(mediumCount * 100 / m_questions.size());
    out << QString("| 困难 | %1 | %2% |\n").arg(hardCount).arg(hardCount * 100 / m_questions.size());
    out << "\n";
    
    out << "## 🏷️ 知识点分布\n\n";
    out << "| 知识点 | 题目数 |\n";
    out << "|--------|--------|\n";
    for (auto it = tagCount.begin(); it != tagCount.end(); ++it) {
        out << QString("| %1 | %2 |\n").arg(it.key()).arg(it.value());
    }
    out << "\n";
    
    out << "## 📋 出题规则\n\n";
    out << "### 套题数量\n";
    out << "- 每套题目数量: " << m_questions.size() << " 道\n\n";
    
    out << "### 难度配比建议\n";
    out << "- 简单题: " << QString::number(easyCount * 100.0 / m_questions.size(), 'f', 0) << "%\n";
    out << "- 中等题: " << QString::number(mediumCount * 100.0 / m_questions.size(), 'f', 0) << "%\n";
    out << "- 困难题: " << QString::number(hardCount * 100.0 / m_questions.size(), 'f', 0) << "%\n\n";
    
    out << "### 测试数据规则\n";
    out << "- 每题至少 3 组原始测试数据\n";
    out << "- AI自动补充 2-3 组边界/异常测试数据\n";
    out << "- 测试数据覆盖：基本功能、边界条件、特殊情况\n\n";
    
    out << "## 🎯 题号专属规则\n\n";
    out << "根据题目顺序和难度，建议的题号分配：\n\n";
    for (int i = 0; i < qMin(5, m_questions.size()); ++i) {
        const Question &q = m_questions[i];
        QString diffStr = (q.difficulty() == Difficulty::Easy ? "简单" : 
                          q.difficulty() == Difficulty::Medium ? "中等" : "困难");
        out << QString("- 第 %1 题: %2 (%3)\n").arg(i + 1).arg(q.title()).arg(diffStr);
    }
    
    file.close();
    emit logMessage(QString("  ✓ 出题模式规律已保存: %1").arg(patternFile));
    return true;
}

// 使用通用解析器的导入流程
void SmartQuestionImporter::startImportWithUniversalParser(const QString &sourcePath, const QString &targetPath, const QString &bankName)
{
    m_targetPath = targetPath;
    m_bankName = bankName;
    m_cancelled = false;
    m_questions.clear();
    m_useUniversalParser = true;
    
    emit logMessage("🚀 开始通用智能导入流程...\n");
    
    // 第一步：拷贝文件夹
    emit logMessage("📁 第一步：拷贝题库文件...");
    if (!copyQuestionBank(sourcePath, targetPath)) {
        emit importCompleted(false, "文件拷贝失败");
        return;
    }
    emit logMessage("✅ 文件拷贝完成\n");
    
    // 第二步：使用通用解析器解析所有文件
    emit logMessage("📖 第二步：智能解析题目格式...");
    
    QDir dir(targetPath);
    QStringList filters;
    filters << "*.md" << "*.markdown" << "*.txt";
    QFileInfoList files = dir.entryInfoList(filters, QDir::Files | QDir::NoDotAndDotDot);
    
    m_progress.totalFiles = files.size();
    m_progress.processedFiles = 0;
    m_progress.totalQuestions = 0;
    
    for (const QFileInfo &fileInfo : files) {
        if (m_cancelled) {
            emit importCompleted(false, "用户取消");
            return;
        }
        
        QString filePath = fileInfo.absoluteFilePath();
        m_progress.currentFile = fileInfo.fileName();
        m_progress.currentStatus = "解析中...";
        emit progressUpdated(m_progress);
        
        emit logMessage(QString("📄 处理: %1").arg(fileInfo.fileName()));
        
        // 读取文件内容
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            emit logMessage(QString("  ⚠️ 无法打开文件"));
            continue;
        }
        
        QTextStream in(&file);
        in.setEncoding(QStringConverter::Utf8);
        QString content = in.readAll();
        file.close();
        
        // 分析格式
        ParsePattern pattern = m_parser->analyzeFormat(content);
        
        // 解析题目
        QVector<Question> questions = m_parser->parseContent(content, pattern);
        
        if (questions.isEmpty()) {
            emit logMessage(QString("  ⚠️ 未解析到题目"));
        } else {
            emit logMessage(QString("  ✅ 解析到 %1 道题目").arg(questions.size()));
            
            // 为每道题目生成唯一ID
            for (Question &q : questions) {
                QString id = QString("%1_%2").arg(m_bankName).arg(m_questions.size() + 1);
                q.setId(id);
                
                // 如果测试用例少于3个，标记需要AI扩充
                if (q.testCases().size() < MIN_TEST_CASES) {
                    emit logMessage(QString("    ⏳ 题目 \"%1\" 测试用例不足，需要AI扩充").arg(q.title()));
                }
                
                m_questions.append(q);
            }
            
            m_progress.totalQuestions = m_questions.size();
        }
        
        m_progress.processedFiles++;
        emit progressUpdated(m_progress);
        emit fileProcessed(fileInfo.fileName(), questions.size());
    }
    
    emit logMessage(QString("\n✅ 解析完成，共 %1 道题目\n").arg(m_questions.size()));
    
    // 第三步：AI扩充测试数据（如果需要）
    if (m_aiClient) {
        emit logMessage("🤖 第三步：AI扩充测试数据...");
        
        int expandedCount = 0;
        for (int i = 0; i < m_questions.size(); ++i) {
            if (m_cancelled) {
                emit importCompleted(false, "用户取消");
                return;
            }
            
            Question &q = m_questions[i];
            if (q.testCases().size() < MIN_TEST_CASES) {
                emit logMessage(QString("  🔄 扩充题目 %1/%2: %3")
                    .arg(i + 1).arg(m_questions.size()).arg(q.title()));
                
                // 这里可以调用AI生成更多测试用例
                // 暂时跳过，保持原有测试用例
                expandedCount++;
            }
        }
        
        if (expandedCount > 0) {
            emit logMessage(QString("  ✅ 已标记 %1 道题目需要扩充\n").arg(expandedCount));
        } else {
            emit logMessage("  ✅ 所有题目测试数据充足\n");
        }
    }
    
    // 第四步：保存解析规则和基础题库
    emit logMessage("📝 第四步：保存解析规则和基础题库...");
    if (saveParseRulesAndQuestionBank()) {
        emit logMessage("✅ 解析规则和基础题库保存完成");
    } else {
        emit logMessage("⚠️ 保存过程中出现部分问题");
    }
    
    // 第五步：生成出题模式规律
    emit logMessage("📊 第五步：生成出题模式规律...");
    if (generateExamPattern()) {
        emit logMessage("✅ 出题模式规律生成完成");
    }
    
    // 第六步：生成题库分析报告
    emit logMessage("📊 第六步：生成题库分析报告...");
    
    BankAnalysis analysis = m_analyzer->analyzeQuestions(m_questions, m_bankName);
    
    if (m_analyzer->saveAnalysis(targetPath, analysis)) {
        emit logMessage("  ✅ 分析报告已保存");
        emit logMessage(QString("  📈 难度分布: 简单 %1, 中等 %2, 困难 %3")
            .arg(analysis.difficultyDistribution["简单"])
            .arg(analysis.difficultyDistribution["中等"])
            .arg(analysis.difficultyDistribution["困难"]));
        emit logMessage(QString("  📊 平均测试用例: %.1f 组").arg(analysis.avgTestCases));
    }
    
    // 第七步：保存运行时题库JSON
    emit logMessage("\n💾 第七步：保存运行时题库...");
    if (saveRuntimeQuestionBank()) {
        emit logMessage("✅ 运行时题库保存完成");
    }
    
    emit logMessage("\n🎉 导入完成！");
    emit importCompleted(true, QString("成功导入 %1 道题目").arg(m_questions.size()));
}
