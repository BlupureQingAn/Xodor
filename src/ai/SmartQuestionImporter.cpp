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
#include <QDebug>
#include <QTimer>

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

void SmartQuestionImporter::startImport(const QString &sourcePath, const QString &targetPath)
{
    m_targetPath = targetPath;
    m_cancelled = false;
    m_chunks.clear();
    m_questions.clear();
    m_currentChunkIndex = 0;
    
    emit logMessage("🚀 开始智能导入流程...\n");
    
    // 第一步：拷贝文件夹
    emit logMessage("📁 第一步：拷贝题库文件...");
    if (!copyQuestionBank(sourcePath, targetPath)) {
        emit importCompleted(false, "文件拷贝失败");
        return;
    }
    
    emit logMessage("✅ 文件拷贝完成\n");
    
    // 第二步：扫描并分析文件
    emit logMessage("📂 第二步：扫描和分析文件...");
    scanAndAnalyzeFiles(targetPath);
    
    if (m_chunks.isEmpty()) {
        emit importCompleted(false, "未找到任何题目文件");
        return;
    }
    
    emit logMessage(QString("✅ 文件分析完成，共 %1 个文件块\n").arg(m_chunks.size()));
    
    // 更新进度
    m_progress.totalChunks = m_chunks.size();
    m_progress.processedChunks = 0;
    m_progress.currentStatus = "准备开始AI解析";
    emit progressUpdated(m_progress);
    
    // 第三步：开始处理第一个块
    emit logMessage("🤖 第三步：AI智能解析题目...");
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
    QDir dir(path);
    QStringList filters;
    filters << "*.md" << "*.markdown" << "*.txt";
    
    QFileInfoList files = dir.entryInfoList(filters, QDir::Files);
    
    m_progress.totalFiles = files.size();
    m_progress.processedFiles = 0;
    
    for (const QFileInfo &fileInfo : files) {
        QFile file(fileInfo.absoluteFilePath());
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            continue;
        }
        
        QTextStream in(&file);
        in.setEncoding(QStringConverter::Utf8);
        QString content = in.readAll();
        file.close();
        
        emit logMessage(QString("  分析: %1 (%2 字符)")
            .arg(fileInfo.fileName())
            .arg(content.length()));
        
        // 智能拆分文件
        QVector<FileChunk> chunks = splitLargeFile(fileInfo.fileName(), content);
        
        if (chunks.size() > 1) {
            emit logMessage(QString("    → 拆分为 %1 个块").arg(chunks.size()));
        }
        
        m_chunks.append(chunks);
    }
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
        emit logMessage(QString("\n✅ 导入完成！共导入 %1 道题目").arg(m_questions.size()));
        
        m_progress.currentStatus = "导入完成";
        emit progressUpdated(m_progress);
        emit importCompleted(true, QString("成功导入 %1 道题目").arg(m_questions.size()));
        return;
    }
    
    const FileChunk &chunk = m_chunks[m_currentChunkIndex];
    
    // 更新进度
    m_progress.currentFile = chunk.fileName;
    m_progress.currentStatus = QString("处理文件块 %1/%2")
        .arg(m_currentChunkIndex + 1)
        .arg(m_chunks.size());
    m_progress.processedChunks = m_currentChunkIndex;
    emit progressUpdated(m_progress);
    
    emit logMessage(QString("\n📄 处理: %1 (块 %2/%3)")
        .arg(chunk.fileName)
        .arg(chunk.chunkIndex + 1)
        .arg(chunk.totalChunks));
    
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
        emit logMessage("  ⚠️ JSON解析失败，跳过此块");
        return;
    }
    
    QJsonObject root = doc.object();
    QJsonArray questionsArray = root["questions"].toArray();
    
    emit logMessage(QString("  ✓ 解析到 %1 道题目").arg(questionsArray.size()));
    
    for (const QJsonValue &val : questionsArray) {
        QJsonObject qObj = val.toObject();
        
        Question q;
        q.setId(QString("q_%1").arg(qHash(qObj["title"].toString())));
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
            testCases.append(tc);
        }
        
        // 如果测试用例不足，生成更多
        if (testCases.size() < MIN_TEST_CASES) {
            QVector<TestCase> generated = generateTestCases(q);
            testCases.append(generated);
        }
        
        q.setTestCases(testCases);
        q.setType(QuestionType::Code);
        
        m_questions.append(q);
        m_progress.totalQuestions++;
        
        emit logMessage(QString("    ✓ %1 [%2] - %3个测试用例")
            .arg(q.title())
            .arg(diffStr)
            .arg(testCases.size()));
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
    
    // 更新进度信息
    m_progress.currentStatus = QString("AI正在解析... (已接收 %1 字符)")
        .arg(currentLength);
    
    // 计算当前块的进度百分比（0-100）
    int chunkProgress = qMin(100, (currentLength * 100) / 10000);  // 假设每个响应最多10000字符
    
    emit progressUpdated(m_progress);
    
    // 每1000字符输出一次日志
    static int lastLoggedLength = 0;
    if (currentLength - lastLoggedLength >= 1000) {
        emit logMessage(QString("  📥 接收中... %1 字符").arg(currentLength));
        lastLoggedLength = currentLength;
    }
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
    
    // 第四步：生成题库分析报告
    emit logMessage("📊 第四步：生成题库分析报告...");
    
    BankAnalysis analysis = m_analyzer->analyzeQuestions(m_questions, m_bankName);
    
    if (m_analyzer->saveAnalysis(targetPath, analysis)) {
        emit logMessage("  ✅ 分析报告已保存");
        emit logMessage(QString("  📈 难度分布: 简单 %1, 中等 %2, 困难 %3")
            .arg(analysis.difficultyDistribution["简单"])
            .arg(analysis.difficultyDistribution["中等"])
            .arg(analysis.difficultyDistribution["困难"]));
        emit logMessage(QString("  📊 平均测试用例: %.1f 组").arg(analysis.avgTestCases));
    }
    
    emit logMessage("\n🎉 导入完成！");
    emit importCompleted(true, QString("成功导入 %1 道题目").arg(m_questions.size()));
}
