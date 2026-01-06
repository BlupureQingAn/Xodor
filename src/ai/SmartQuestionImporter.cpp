#include "SmartQuestionImporter.h"
#include "OllamaClient.h"
#include "UniversalQuestionParser.h"
#include "QuestionBankAnalyzer.h"
#include "../utils/ImportRuleManager.h"
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
    , m_isRecursiveProcessing(false)
    , m_recursiveDepth(0)
    , m_lastContentLength(0)
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
        emit importCompleted(buildImportResult(false, "原始题库备份失败"));
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
        emit importCompleted(buildImportResult(false, "未找到任何题目文件"));
        return;
    }
    
    // 更新进度
    m_progress.currentStatus = "开始AI递归拆分识别";
    emit progressUpdated(m_progress);
    
    // 开始处理第一个块
    emit logMessage("\n[2/2] 🤖 AI解析并实时保存...");
    processNextChunk();
}

void SmartQuestionImporter::cancelImport()
{
    m_cancelled = true;
    
    // 终止正在进行的AI请求
    if (m_aiClient) {
        m_aiClient->abortCurrentRequest();
        emit logMessage("\n⚠️ 用户取消导入，正在终止AI请求...");
    } else {
        emit logMessage("\n⚠️ 用户取消导入");
    }
    
    emit importCompleted(buildImportResult(false, "用户取消"));
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
    
    // 进入扫描阶段
    enterScanningStage();
    
    QDir dir(path);
    QStringList filters;
    filters << "*.md" << "*.markdown" << "*.txt";
    
    QFileInfoList files = dir.entryInfoList(filters, QDir::Files);
    
    m_progress.totalFiles = files.size();
    m_progress.processedFiles = 0;
    
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
        updateProgress();
    }
    
    emit logMessage(QString("\n  共 %1 个文件，开始AI递归拆分识别\n")
        .arg(files.size()));
    
    // 扫描完成，进入解析阶段
    enterParsingStage();
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
        // 所有块处理完成，进入保存阶段
        emit logMessage(QString("\n✅ AI解析完成！共导入 %1 道题目").arg(m_questions.size()));
        
        enterSavingStage();
        
        // 第四步：保存解析规则和基础题库
        emit logMessage("\n📝 第四步：保存解析规则和基础题库...");
        m_progress.saveProgress = 30;
        updateProgress();
        
        if (saveParseRulesAndQuestionBank()) {
            emit logMessage("✅ 解析规则和基础题库保存完成");
        } else {
            emit logMessage("⚠️ 保存过程中出现部分问题");
        }
        
        m_progress.saveProgress = 70;
        updateProgress();
        
        // 第五步：生成出题模式规律
        emit logMessage("\n📊 第五步：生成出题模式规律...");
        if (generateExamPattern()) {
            emit logMessage("✅ 出题模式规律生成完成");
        }
        
        m_progress.saveProgress = 100;
        updateProgress();
        
        // 进入完成阶段
        enterCompleteStage();
        
        emit importCompleted(buildImportResult(true));
        return;
    }
    
    const FileChunk &chunk = m_chunks[m_currentChunkIndex];
    
    // 保存当前正在处理的chunk
    m_currentProcessingChunk = chunk;
    
    // 重置当前文件的处理状态
    m_processedTitles.clear();
    m_recursiveDepth = 0;
    m_lastContentLength = chunk.content.length();
    
    // 更新进度
    m_progress.currentFile = chunk.fileName;
    m_progress.currentFileIndex = m_currentChunkIndex;  // 设置当前文件索引
    m_progress.currentStatus = QString("AI递归拆分 %1/%2 - 已识别 %3 道题目")
        .arg(m_currentChunkIndex + 1)
        .arg(m_chunks.size())
        .arg(m_progress.totalQuestions);
    updateProgress();
    
    emit logMessage(QString("\n[%1/%2] 📄 %3")
        .arg(m_currentChunkIndex + 1)
        .arg(m_chunks.size())
        .arg(chunk.fileName));
    
    // 发送给AI解析
    parseChunkWithAI(chunk);
}

void SmartQuestionImporter::parseChunkWithAI(const FileChunk &chunk)
{
    qDebug() << "[SmartQuestionImporter] parseChunkWithAI 开始";
    
    if (!m_aiClient) {
        qDebug() << "[SmartQuestionImporter] AI客户端为空!";
        emit logMessage("❌ AI客户端未初始化");
        emit importCompleted(buildImportResult(false, "AI客户端未初始化"));
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
    // 为内容添加行号
    QStringList lines = chunk.content.split('\n');
    QString numberedContent;
    for (int i = 0; i < lines.size(); ++i) {
        numberedContent += QString("%1: %2\n").arg(i + 1, 4).arg(lines[i]);
    }
    
    QString prompt = R"(
你是编程题目分析助手。

【核心任务】
分析文档，识别第一道题目，返回JSON格式的提取指令。

【重要原则】
1. **不要输出题目的完整内容** - 只输出提取指令和元数据
2. **使用行号指定范围** - 题目内容由程序从原文件提取
3. **只处理第一道题** - 不要尝试处理所有题目

【题目识别标志】
- 一级标题（# 题目名）
- 题号（1. 题目、第1题、题目1）
- 分隔线后的标题

【输出格式 - 必须是纯JSON】
{
  "action": "extract_first_question",
  "question": {
    "title": "题目标题",
    "difficulty": "简单/中等/困难",
    "tags": ["标签1", "标签2"],
    "content_range": {
      "start_line": 起始行号,
      "end_line": 结束行号
    },
    "test_cases_hints": [
      {
        "type": "基本功能",
        "input_hint": "实际的输入数据（可直接复制粘贴到程序）",
        "output_hint": "实际的输出数据（可直接复制粘贴验证）"
      },
      {
        "type": "边界条件",
        "input_hint": "实际的边界输入数据",
        "output_hint": "实际的边界输出数据"
      }
    ]
  },
  "remaining": {
    "start_line": 剩余内容起始行号,
    "has_more_questions": true/false,
    "estimated_count": 估计剩余题目数量
  }
}

【示例】
输入文档（带行号）：
   1: # 1. 两数之和
   2: 
   3: 给定一个整数数组 nums 和一个整数目标值 target
   4: 
   5: 输入：nums = [2,7,11,15], target = 9
   6: 输出：[0,1]
   7: 
   8: # 2. 三数之和
   9: 
  10: 给定一个包含n个整数的数组...

输出JSON：
{
  "action": "extract_first_question",
  "question": {
    "title": "两数之和",
    "difficulty": "简单",
    "tags": ["数组", "哈希表"],
    "content_range": {
      "start_line": 1,
      "end_line": 7
    },
    "test_cases_hints": [
      {
        "type": "基本功能",
        "input_hint": "2 7 11 15\n9",
        "output_hint": "0 1"
      },
      {
        "type": "边界条件",
        "input_hint": "3 3\n6",
        "output_hint": "0 1"
      }
    ]
  },
  "remaining": {
    "start_line": 8,
    "has_more_questions": true,
    "estimated_count": 1
  }
}

文档内容（带行号）：
---
)";
    
    prompt += numberedContent;
    prompt += R"(
---

【重要提醒】
1. 只输出纯JSON，不要任何其他文字
2. 使用行号指定内容范围
3. **test_cases_hints必须是实际的测试数据，可以直接复制粘贴到程序中运行**
   - input_hint: 实际的输入数据（如：3 2\n10 10\n0 0\n10 -20\n1 -1\n0 0）
   - output_hint: 实际的输出数据（如：21 -11\n10 -20）
   - 不要写描述性文字（如"输出两个坐标..."）
   - 不要添加"代码"、"输入："等前缀
4. 确保JSON格式正确

现在输出JSON：
)";
    
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
    
    emit logMessage(QString("  ✓ AI响应接收完成 (%1 字符)").arg(response.length()));
    
    // 显示响应的前几行和后几行，帮助诊断
    QStringList lines = response.split('\n');
    if (lines.size() > 0) {
        emit logMessage(QString("  📝 响应开头: %1...").arg(lines.first().left(80)));
        if (lines.size() > 1) {
            emit logMessage(QString("  📝 第2行: %1...").arg(lines[1].left(80)));
        }
        if (lines.size() > 5) {
            emit logMessage(QString("  📝 最后一行: %1").arg(lines.last().left(80)));
        }
    }
    
    // 使用递归拆分策略处理响应
    // 使用当前正在处理的chunk，而不是m_chunks[m_currentChunkIndex]
    // 因为在递归处理时，m_currentProcessingChunk会被更新为剩余内容
    parseAIResponseRecursive(response, m_currentProcessingChunk);
    
    // 注意：parseAIResponseRecursive 会在内部决定是否继续递归或进入下一个文件
    // 不要在这里调用 processNextChunk()，否则会导致递归还没完成就进入下一个文件
}

void SmartQuestionImporter::parseAIResponseAndGenerateTests(const QString &response, const FileChunk &chunk)
{
    // 显示响应的基本信息
    emit logMessage(QString("  📊 AI响应总长度: %1 字符").arg(response.length()));
    
    // 保存原始响应用于调试（可选）
    static bool saveDebugResponse = true;  // 设置为false可禁用
    if (saveDebugResponse) {
        QString debugDir = "debug_ai_responses";
        QDir dir;
        if (!dir.exists(debugDir)) {
            dir.mkpath(debugDir);
        }
        
        QString safeFileName = chunk.fileName;
        safeFileName.replace(QRegularExpression("[\\\\/:*?\"<>|]"), "_");
        QString debugFile = QString("%1/response_%2_%3.txt")
            .arg(debugDir)
            .arg(safeFileName)
            .arg(QDateTime::currentMSecsSinceEpoch());
        
        QFile file(debugFile);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out.setEncoding(QStringConverter::Utf8);
            out << response;
            file.close();
            emit logMessage(QString("  🐛 调试：响应已保存到 %1").arg(debugFile));
        }
    }
    
    // 统计分隔符数量
    int separatorCount = response.count("---QUESTION_SEPARATOR---");
    emit logMessage(QString("  🔍 找到 %1 个分隔符 (---QUESTION_SEPARATOR---)").arg(separatorCount));
    emit logMessage(QString("  💡 预期题目数量: %1 道").arg(separatorCount + 1));
    
    // 检查是否有其他可能的分隔符变体
    QString actualSeparator = "---QUESTION_SEPARATOR---";
    if (separatorCount == 0) {
        int variant1 = response.count("--- QUESTION SEPARATOR ---");
        int variant2 = response.count("---question_separator---");
        int variant3 = response.count("===QUESTION_SEPARATOR===");
        int variant4 = response.count("---QUESTION-SEPARATOR---");
        int variant5 = response.count("--- QUESTION_SEPARATOR ---");
        
        if (variant1 > 0) {
            emit logMessage(QString("  ⚠️ 发现 %1 个变体分隔符: '--- QUESTION SEPARATOR ---' (有空格)").arg(variant1));
            actualSeparator = "--- QUESTION SEPARATOR ---";
            separatorCount = variant1;
        } else if (variant2 > 0) {
            emit logMessage(QString("  ⚠️ 发现 %1 个变体分隔符: '---question_separator---' (小写)").arg(variant2));
            actualSeparator = "---question_separator---";
            separatorCount = variant2;
        } else if (variant3 > 0) {
            emit logMessage(QString("  ⚠️ 发现 %1 个变体分隔符: '===QUESTION_SEPARATOR===' (不同符号)").arg(variant3));
            actualSeparator = "===QUESTION_SEPARATOR===";
            separatorCount = variant3;
        } else if (variant4 > 0) {
            emit logMessage(QString("  ⚠️ 发现 %1 个变体分隔符: '---QUESTION-SEPARATOR---' (连字符)").arg(variant4));
            actualSeparator = "---QUESTION-SEPARATOR---";
            separatorCount = variant4;
        } else if (variant5 > 0) {
            emit logMessage(QString("  ⚠️ 发现 %1 个变体分隔符: '--- QUESTION_SEPARATOR ---' (前后空格)").arg(variant5));
            actualSeparator = "--- QUESTION_SEPARATOR ---";
            separatorCount = variant5;
        }
        
        if (separatorCount > 0) {
            emit logMessage(QString("  💡 使用变体分隔符，预期题目数量: %1 道").arg(separatorCount + 1));
        }
    }
    
    // 按分隔符拆分题目
    QStringList questionBlocks = response.split(actualSeparator, Qt::SkipEmptyParts);
    
    if (questionBlocks.isEmpty()) {
        emit logMessage("  ⚠️ 未找到题目分隔符，尝试作为单个题目处理...");
        questionBlocks.append(response);
    }
    
    emit logMessage(QString("  ✓ 实际识别到 %1 道题目，开始解析并保存...").arg(questionBlocks.size()));
    
    // 准备保存目录
    QString sourceFileName = chunk.fileName;
    sourceFileName = QFileInfo(sourceFileName).baseName();
    QString baseQuestionBankDir = QString("data/基础题库/%1").arg(m_bankName);
    
    QDir dir;
    if (!dir.mkpath(baseQuestionBankDir)) {
        emit logMessage(QString("  ❌ 无法创建目录: %1").arg(baseQuestionBankDir));
        return;
    }
    
    int successCount = 0;
    int blockIndex = 0;
    for (const QString &block : questionBlocks) {
        blockIndex++;
        QString trimmedBlock = block.trimmed();
        
        emit logMessage(QString("  📄 处理第 %1/%2 个块，长度: %3 字符")
            .arg(blockIndex)
            .arg(questionBlocks.size())
            .arg(trimmedBlock.length()));
        
        if (trimmedBlock.isEmpty()) {
            emit logMessage(QString("  ⚠️ 第 %1 个块为空，跳过").arg(blockIndex));
            continue;
        }
        
        // 解析Front Matter
        QRegularExpression frontMatterRegex("^---\\s*\\n(.+?)\\n---\\s*\\n(.*)$", 
                                           QRegularExpression::DotMatchesEverythingOption);
        QRegularExpressionMatch match = frontMatterRegex.match(trimmedBlock);
        
        if (!match.hasMatch()) {
            emit logMessage(QString("  ⚠️ 第 %1 个块格式不正确，跳过").arg(blockIndex));
            emit logMessage(QString("  📝 块开头: %1...").arg(trimmedBlock.left(100).replace('\n', ' ')));
            continue;
        }
        
        QString frontMatter = match.captured(1);
        QString content = match.captured(2);
        
        // 解析Front Matter字段（支持带引号和不带引号）
        QString title, difficulty;
        QStringList tags;
        
        // 匹配title（支持带引号和不带引号）
        QRegularExpression titleRegex("title:\\s*(?:[\"'](.+?)[\"']|([^\\n]+))");
        QRegularExpressionMatch titleMatch = titleRegex.match(frontMatter);
        if (titleMatch.hasMatch()) {
            title = titleMatch.captured(1).isEmpty() ? 
                    titleMatch.captured(2).trimmed() : 
                    titleMatch.captured(1);
        }
        
        // 匹配difficulty（支持带引号和不带引号）
        QRegularExpression diffRegex("difficulty:\\s*(?:[\"'](.+?)[\"']|([^\\n]+))");
        QRegularExpressionMatch diffMatch = diffRegex.match(frontMatter);
        if (diffMatch.hasMatch()) {
            difficulty = diffMatch.captured(1).isEmpty() ? 
                        diffMatch.captured(2).trimmed() : 
                        diffMatch.captured(1);
        }
        
        QRegularExpression tagsRegex("tags:\\s*\\[(.+?)\\]");
        QRegularExpressionMatch tagsMatch = tagsRegex.match(frontMatter);
        if (tagsMatch.hasMatch()) {
            QString tagsStr = tagsMatch.captured(1);
            QRegularExpression tagItemRegex("[\"']([^\"']+)[\"']");
            QRegularExpressionMatchIterator it = tagItemRegex.globalMatch(tagsStr);
            while (it.hasNext()) {
                QRegularExpressionMatch tagMatch = it.next();
                tags.append(tagMatch.captured(1));
            }
        }
        
        if (title.isEmpty()) {
            emit logMessage(QString("  ⚠️ 第 %1 个块缺少标题，跳过").arg(blockIndex));
            emit logMessage(QString("  📝 Front Matter: %1").arg(frontMatter.left(200)));
            continue;
        }
        
        emit logMessage(QString("  ✓ 第 %1 个块: %2 [%3]")
            .arg(blockIndex)
            .arg(title)
            .arg(difficulty.isEmpty() ? "未知难度" : difficulty));
        
        // 创建Question对象
        Question q;
        q.setId(QString("%1_%2").arg(sourceFileName).arg(qHash(title)));
        q.setTitle(title);
        q.setDescription(content);  // 完整的原文内容
        q.setTags(tags);
        
        // 解析难度
        if (difficulty.contains("简单") || difficulty.contains("easy", Qt::CaseInsensitive)) {
            q.setDifficulty(Difficulty::Easy);
        } else if (difficulty.contains("困难") || difficulty.contains("hard", Qt::CaseInsensitive)) {
            q.setDifficulty(Difficulty::Hard);
        } else {
            q.setDifficulty(Difficulty::Medium);
        }
        
        // 从content中提取测试用例
        QVector<TestCase> testCases = extractTestCasesFromMarkdown(content);
        q.setTestCases(testCases);
        q.setType(QuestionType::Code);
        
        // 确定保存的子目录（按源文件分类，而不是按难度）
        // 同一个源文件拆分出来的题目放在同一个文件夹
        QString subDir = QString("%1/%2").arg(baseQuestionBankDir).arg(sourceFileName);
        if (!dir.mkpath(subDir)) {
            emit logMessage(QString("  ❌ 无法创建目录: %1").arg(subDir));
            continue;
        }
        
        // 生成安全的文件名
        QString safeTitle = title;
        safeTitle.replace(QRegularExpression("[\\\\/:*?\"<>|]"), "_");
        safeTitle = safeTitle.trimmed();
        if (safeTitle.isEmpty()) {
            safeTitle = QString("题目%1").arg(m_progress.totalQuestions + 1);
        }
        
        // 保存为MD文件
        QString mdFilePath = QString("%1/%2.md").arg(subDir).arg(safeTitle);
        bool isOverwrite = QFile::exists(mdFilePath);
        
        if (q.saveAsMarkdown(mdFilePath)) {
            m_questions.append(q);
            m_progress.totalQuestions++;
            successCount++;
            
            QString diffEmoji = (q.difficulty() == Difficulty::Easy) ? "🟢" : 
                               (q.difficulty() == Difficulty::Hard) ? "🔴" : "🟡";
            QString saveStatus = isOverwrite ? "✓已覆盖" : "✓已保存";
            emit logMessage(QString("    %1 %2 [%3] - %4个测试用例 %5")
                .arg(diffEmoji)
                .arg(title)
                .arg(difficulty)
                .arg(testCases.size())
                .arg(saveStatus));
        } else {
            emit logMessage(QString("    ❌ 保存失败: %1").arg(title));
        }
    }
    
    emit logMessage(QString("  ✅ 成功保存 %1 道题目").arg(successCount));
}

QVector<TestCase> SmartQuestionImporter::extractTestCasesFromMarkdown(const QString &markdown)
{
    QVector<TestCase> testCases;
    
    // 查找测试用例部分
    QRegularExpression testCaseHeaderRegex("##\\s*测试用例");
    QRegularExpressionMatch headerMatch = testCaseHeaderRegex.match(markdown);
    int testCaseStart = headerMatch.capturedStart();
    
    if (testCaseStart < 0) {
        return testCases;  // 没有测试用例部分
    }
    
    QString testCaseSection = markdown.mid(testCaseStart);
    
    // 匹配每个测试用例
    QRegularExpression testCaseRegex(
        "###\\s*测试用例\\s*\\d+\\s*\\n"
        "\\*\\*输入\\*\\*:\\s*\\n```\\n(.+?)\\n```\\s*\\n"
        "\\*\\*输出\\*\\*:\\s*\\n```\\n(.+?)\\n```\\s*\\n"
        "(?:\\*\\*说明\\*\\*:\\s*(.+?)\\n)?",
        QRegularExpression::DotMatchesEverythingOption
    );
    
    QRegularExpressionMatchIterator it = testCaseRegex.globalMatch(testCaseSection);
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        TestCase tc;
        tc.input = match.captured(1).trimmed();
        tc.expectedOutput = match.captured(2).trimmed();
        tc.description = match.captured(3).trimmed();
        tc.isAIGenerated = true;
        testCases.append(tc);
    }
    
    return testCases;
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
    
    // 1. 统计题目信息
    int easyCount = 0, mediumCount = 0, hardCount = 0;
    int totalTestCases = 0;
    
    for (const Question &q : m_questions) {
        switch (q.difficulty()) {
            case Difficulty::Easy: easyCount++; break;
            case Difficulty::Medium: mediumCount++; break;
            case Difficulty::Hard: hardCount++; break;
        }
        totalTestCases += q.testCases().size();
    }
    
    double avgTestCases = m_questions.size() > 0 ? 
        totalTestCases * 1.0 / m_questions.size() : 0;
    
    // 2. 构建解析规则JSON对象
    QJsonObject parseRule;
    parseRule["bankName"] = m_bankName;
    parseRule["createdTime"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    parseRule["parseMode"] = "AI智能解析";
    
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
    
    // 添加统计信息
    QJsonObject statistics;
    statistics["totalQuestions"] = m_questions.size();
    QJsonObject diffDist;
    diffDist["简单"] = easyCount;
    diffDist["中等"] = mediumCount;
    diffDist["困难"] = hardCount;
    statistics["difficultyDistribution"] = diffDist;
    statistics["avgTestCases"] = avgTestCases;
    parseRule["statistics"] = statistics;
    
    // 3. 使用ImportRuleManager保存规则文件到config目录
    if (ImportRuleManager::saveImportRule(m_bankName, parseRule)) {
        QString rulePath = ImportRuleManager::getRulePath(m_bankName);
        emit logMessage(QString("  ✓ 解析规则已保存: %1").arg(rulePath));
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
    // 题目已经保存为独立的MD文件，直接使用即可
    emit logMessage("  ℹ️ 运行时直接使用基础题库MD文件");
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
            // 过滤掉"测试用例"标签，不应该显示在知识点分布中
            if (tag != "测试用例" && tag != "test case" && tag != "Test Case") {
                tagCount[tag]++;
            }
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
        emit importCompleted(buildImportResult(false, "文件拷贝失败"));
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
            emit importCompleted(buildImportResult(false, "用户取消"));
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
                emit importCompleted(buildImportResult(false, "用户取消"));
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
    emit importCompleted(buildImportResult(true));
}


// ==================== 递归拆分相关方法 ====================

void SmartQuestionImporter::parseAIResponseRecursive(const QString &response, const FileChunk &chunk)
{
    emit logMessage("  📋 解析AI指令...");
    
    // 检查递归深度，防止无限循环
    const int MAX_RECURSIVE_DEPTH = 20;  // 最多20道题
    if (m_recursiveDepth >= MAX_RECURSIVE_DEPTH) {
        emit logMessage(QString("  ⚠️ 达到最大递归深度 (%1)，停止处理当前文件").arg(MAX_RECURSIVE_DEPTH));
        m_isRecursiveProcessing = false;
        m_recursiveDepth = 0;
        m_processedTitles.clear();
        m_currentChunkIndex++;
        processNextChunk();
        return;
    }
    
    // 获取当前内容长度
    int currentContentLength = chunk.content.length();
    
    emit logMessage(QString("  📊 当前递归深度: %1, 内容长度: %2 字符, 上次长度: %3 字符")
        .arg(m_recursiveDepth).arg(currentContentLength).arg(m_lastContentLength));
    
    // 检查内容长度是否在减少（只在递归处理时检查）
    if (m_isRecursiveProcessing && currentContentLength >= m_lastContentLength) {
        emit logMessage(QString("  ⚠️ 检测到内容长度未减少 (当前:%1, 上次:%2)，可能陷入循环，停止处理")
            .arg(currentContentLength).arg(m_lastContentLength));
        m_isRecursiveProcessing = false;
        m_recursiveDepth = 0;
        m_processedTitles.clear();
        m_currentChunkIndex++;
        processNextChunk();
        return;
    }
    
    // 更新内容长度记录（在检查通过后立即更新，为下次检查做准备）
    m_lastContentLength = currentContentLength;
    
    // 提取JSON部分（AI可能在前后添加了说明文字）
    QString jsonStr = response.trimmed();
    int jsonStart = jsonStr.indexOf('{');
    int jsonEnd = jsonStr.lastIndexOf('}');
    
    if (jsonStart < 0 || jsonEnd < 0 || jsonEnd <= jsonStart) {
        emit logMessage("  ❌ 未找到有效的JSON指令");
        emit logMessage(QString("  📝 响应内容: %1").arg(response.left(200)));
        // 错误时继续下一个文件
        m_isRecursiveProcessing = false;
        m_recursiveDepth = 0;
        m_processedTitles.clear();
        m_currentChunkIndex++;
        processNextChunk();
        return;
    }
    
    jsonStr = jsonStr.mid(jsonStart, jsonEnd - jsonStart + 1);
    
    // 解析JSON指令
    QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());
    if (doc.isNull() || !doc.isObject()) {
        emit logMessage("  ❌ JSON格式错误");
        emit logMessage(QString("  📝 JSON内容: %1").arg(jsonStr.left(200)));
        // 错误时继续下一个文件
        m_isRecursiveProcessing = false;
        m_recursiveDepth = 0;
        m_processedTitles.clear();
        m_currentChunkIndex++;
        processNextChunk();
        return;
    }
    
    QJsonObject instruction = doc.object();
    QString action = instruction["action"].toString();
    
    if (action != "extract_first_question") {
        emit logMessage(QString("  ❌ 未知的操作类型: %1").arg(action));
        // 错误时继续下一个文件
        m_isRecursiveProcessing = false;
        m_recursiveDepth = 0;
        m_processedTitles.clear();
        m_currentChunkIndex++;
        processNextChunk();
        return;
    }
    
    // 提取题目信息
    QJsonObject questionInfo = instruction["question"].toObject();
    QString title = questionInfo["title"].toString();
    QString difficulty = questionInfo["difficulty"].toString();
    
    if (title.isEmpty()) {
        emit logMessage("  ❌ 题目标题为空");
        // 错误时继续下一个文件
        m_isRecursiveProcessing = false;
        m_recursiveDepth = 0;
        m_processedTitles.clear();
        m_currentChunkIndex++;
        processNextChunk();
        return;
    }
    
    // 检查是否已处理过这个题目（防止重复）
    if (m_processedTitles.contains(title)) {
        emit logMessage(QString("  ⚠️ 题目 \"%1\" 已处理过，跳过（AI识别重复，可能陷入循环）").arg(title));
        emit logMessage(QString("  📋 已处理的题目列表: %1").arg(QStringList(m_processedTitles.begin(), m_processedTitles.end()).join(", ")));
        // 停止处理当前文件，继续下一个文件
        m_isRecursiveProcessing = false;
        m_recursiveDepth = 0;
        m_processedTitles.clear();
        m_currentChunkIndex++;
        processNextChunk();
        return;
    }
    
    emit logMessage(QString("  ✓ 识别到题目: %1 [%2]").arg(title).arg(difficulty));
    
    // 记录已处理的题目
    m_processedTitles.insert(title);
    m_recursiveDepth++;
    
    emit logMessage(QString("  📝 已处理题目数: %1").arg(m_processedTitles.size()));
    
    // 提取内容范围
    QJsonObject contentRange = questionInfo["content_range"].toObject();
    int startLine = contentRange["start_line"].toInt();
    int endLine = contentRange["end_line"].toInt();
    
    if (startLine <= 0 || endLine <= 0 || endLine < startLine) {
        emit logMessage(QString("  ❌ 行号范围无效: %1-%2").arg(startLine).arg(endLine));
        return;
    }
    
    emit logMessage(QString("  📍 内容范围: 第 %1 行到第 %2 行").arg(startLine).arg(endLine));
    
    // 从原文件提取内容
    QString questionContent = extractLines(chunk.content, startLine, endLine);
    
    if (questionContent.isEmpty()) {
        emit logMessage("  ❌ 提取的内容为空");
        return;
    }
    
    emit logMessage(QString("  ✓ 提取内容长度: %1 字符").arg(questionContent.length()));
    
    // 解析标签
    QStringList tags;
    QJsonArray tagsArray = questionInfo["tags"].toArray();
    for (const QJsonValue &val : tagsArray) {
        tags.append(val.toString());
    }
    
    // 创建Question对象
    Question q;
    QString sourceFileName = QFileInfo(chunk.fileName).baseName();
    q.setId(QString("%1_%2").arg(sourceFileName).arg(qHash(title)));
    q.setTitle(title);
    q.setDescription(questionContent);
    q.setTags(tags);
    
    // 解析难度
    if (difficulty.contains("简单") || difficulty.contains("easy", Qt::CaseInsensitive)) {
        q.setDifficulty(Difficulty::Easy);
    } else if (difficulty.contains("困难") || difficulty.contains("hard", Qt::CaseInsensitive)) {
        q.setDifficulty(Difficulty::Hard);
    } else {
        q.setDifficulty(Difficulty::Medium);
    }
    
    // 生成测试用例（基于AI的提示）
    QJsonArray testCasesHints = questionInfo["test_cases_hints"].toArray();
    QVector<TestCase> testCases = generateTestCasesFromHints(testCasesHints, questionContent);
    q.setTestCases(testCases);
    q.setType(QuestionType::Code);
    
    // 保存题目（按源文件分类，而不是按难度）
    QString baseQuestionBankDir = QString("data/基础题库/%1").arg(m_bankName);
    // sourceFileName 已在上面定义，这里直接使用
    
    // 同一个源文件拆分出来的题目放在同一个文件夹
    QString subDir = QString("%1/%2").arg(baseQuestionBankDir).arg(sourceFileName);
    QDir dir;
    if (!dir.mkpath(subDir)) {
        emit logMessage(QString("  ❌ 无法创建目录: %1").arg(subDir));
        return;
    }
    
    // 生成安全的文件名
    QString safeTitle = title;
    safeTitle.replace(QRegularExpression("[\\\\/:*?\"<>|]"), "_");
    safeTitle = safeTitle.trimmed();
    if (safeTitle.isEmpty()) {
        safeTitle = QString("题目%1").arg(m_progress.totalQuestions + 1);
    }
    
    // 保存为MD文件
    QString mdFilePath = QString("%1/%2.md").arg(subDir).arg(safeTitle);
    bool isOverwrite = QFile::exists(mdFilePath);
    
    if (q.saveAsMarkdown(mdFilePath)) {
        m_questions.append(q);
        m_progress.totalQuestions++;
        
        QString diffEmoji = (q.difficulty() == Difficulty::Easy) ? "🟢" : 
                           (q.difficulty() == Difficulty::Hard) ? "🔴" : "🟡";
        QString saveStatus = isOverwrite ? "✓已覆盖" : "✓已保存";
        emit logMessage(QString("    %1 %2 [%3] - %4个测试用例 %5")
            .arg(diffEmoji)
            .arg(title)
            .arg(difficulty)
            .arg(testCases.size())
            .arg(saveStatus));
    } else {
        emit logMessage(QString("    ❌ 保存失败: %1").arg(title));
        return;
    }
    
    // 检查是否还有剩余内容
    QJsonObject remaining = instruction["remaining"].toObject();
    bool hasMore = remaining["has_more_questions"].toBool();
    int estimatedCount = remaining["estimated_count"].toInt();
    
    emit logMessage(QString("  📊 剩余信息: hasMore=%1, estimatedCount=%2")
        .arg(hasMore ? "true" : "false").arg(estimatedCount));
    
    if (hasMore && estimatedCount > 0) {
        int remainingStartLine = remaining["start_line"].toInt();
        
        emit logMessage(QString("  📍 剩余内容起始行: %1").arg(remainingStartLine));
        
        if (remainingStartLine > 0) {
            // 提取剩余内容
            QString remainingContent = extractLinesFrom(chunk.content, remainingStartLine);
            
            emit logMessage(QString("  📏 剩余内容长度: %1 字符").arg(remainingContent.length()));
            
            if (!remainingContent.trimmed().isEmpty()) {
                emit logMessage(QString("  ➡️ 继续处理剩余 %1 道题...").arg(estimatedCount));
                
                // 设置递归处理标志
                m_isRecursiveProcessing = true;
                
                // 创建新的chunk继续处理
                FileChunk newChunk = chunk;
                newChunk.content = remainingContent;
                
                // 更新当前正在处理的chunk（重要！这样onAIResponse才能拿到正确的chunk）
                m_currentProcessingChunk = newChunk;
                
                emit logMessage(QString("  📏 准备递归处理，新chunk长度: %1 字符").arg(newChunk.content.length()));
                
                // 递归调用AI处理剩余内容
                // 注意：不要在这里更新m_lastContentLength，它会在下次parseAIResponseRecursive开始时更新
                parseChunkWithAI(newChunk);
            } else {
                emit logMessage("  ✅ 剩余内容为空，当前文件处理完成");
                // 递归处理完成，继续下一个文件
                m_isRecursiveProcessing = false;
                m_recursiveDepth = 0;
                m_processedTitles.clear();
                m_currentChunkIndex++;
                processNextChunk();
            }
        } else {
            emit logMessage("  ⚠️ 剩余内容起始行号无效");
            // 递归处理完成，继续下一个文件
            m_isRecursiveProcessing = false;
            m_recursiveDepth = 0;
            m_processedTitles.clear();
            m_currentChunkIndex++;
            processNextChunk();
        }
    } else {
        emit logMessage("  ✅ 当前文件所有题目处理完成");
        // 递归处理完成，继续下一个文件
        m_isRecursiveProcessing = false;
        m_recursiveDepth = 0;
        m_processedTitles.clear();
        m_currentChunkIndex++;
        processNextChunk();
    }
}

QString SmartQuestionImporter::extractLines(const QString &content, int startLine, int endLine)
{
    QStringList lines = content.split('\n');
    QStringList extracted;
    
    // 行号从1开始，数组索引从0开始
    for (int i = startLine - 1; i < endLine && i < lines.size(); ++i) {
        extracted.append(lines[i]);
    }
    
    return extracted.join('\n');
}

QString SmartQuestionImporter::extractLinesFrom(const QString &content, int startLine)
{
    QStringList lines = content.split('\n');
    QStringList extracted;
    
    // 行号从1开始，数组索引从0开始
    for (int i = startLine - 1; i < lines.size(); ++i) {
        extracted.append(lines[i]);
    }
    
    return extracted.join('\n');
}

QVector<TestCase> SmartQuestionImporter::generateTestCasesFromHints(const QJsonArray &hints, const QString &questionContent)
{
    QVector<TestCase> testCases;
    
    // 辅助函数：清理测试用例文本
    auto cleanTestCaseText = [](const QString &text) -> QString {
        QString cleaned = text.trimmed();
        
        // 移除常见的无用前缀
        QStringList prefixesToRemove = {
            "代码", "输入代码", "输出代码",
            "请输入", "请输出",
            "输入：", "输出：",
            "输入:", "输出:"
        };
        
        for (const QString &prefix : prefixesToRemove) {
            if (cleaned.startsWith(prefix)) {
                cleaned = cleaned.mid(prefix.length()).trimmed();
            }
        }
        
        // 移除注释行（以//开头的占位符）
        if (cleaned.startsWith("//")) {
            return QString();  // 返回空字符串表示这是无效的占位符
        }
        
        // 移除描述性文字（如"输出两个坐标经过所有平移操作后的结果"）
        // 如果文本不包含具体数据（数字、符号等），可能是描述而非实际测试数据
        QRegularExpression hasDataRegex("[0-9\\[\\]\\(\\)\\{\\},\\-\\+]");
        if (!hasDataRegex.match(cleaned).hasMatch() && cleaned.length() > 20) {
            // 长文本且不包含数据，可能是描述
            qDebug() << "[SmartQuestionImporter] 检测到描述性文本，跳过:" << cleaned.left(30);
            return QString();
        }
        
        return cleaned;
    };
    
    // 从题目内容中提取样例数据
    QRegularExpression inputRegex("输入[：:](.*?)(?=输出|$)", QRegularExpression::DotMatchesEverythingOption);
    QRegularExpression outputRegex("输出[：:](.*?)(?=\\n\\n|$)", QRegularExpression::DotMatchesEverythingOption);
    
    QRegularExpressionMatch inputMatch = inputRegex.match(questionContent);
    QRegularExpressionMatch outputMatch = outputRegex.match(questionContent);
    
    // 如果题目中有样例，使用样例数据
    if (inputMatch.hasMatch() && outputMatch.hasMatch()) {
        QString rawInput = inputMatch.captured(1).trimmed();
        QString rawOutput = outputMatch.captured(1).trimmed();
        
        // 清理文本
        QString cleanedInput = cleanTestCaseText(rawInput);
        QString cleanedOutput = cleanTestCaseText(rawOutput);
        
        // 只有清理后的文本有效才添加测试用例
        if (!cleanedInput.isEmpty() && !cleanedOutput.isEmpty()) {
            TestCase tc;
            tc.input = cleanedInput;
            tc.expectedOutput = cleanedOutput;
            tc.description = "题目样例";
            tc.isAIGenerated = false;
            testCases.append(tc);
            
            qDebug() << "[SmartQuestionImporter] 提取题目样例:";
            qDebug() << "  输入:" << cleanedInput.left(50);
            qDebug() << "  输出:" << cleanedOutput.left(50);
        } else {
            qDebug() << "[SmartQuestionImporter] 题目样例无效，已跳过";
        }
    }
    
    // 根据AI的提示生成额外的测试用例
    for (const QJsonValue &val : hints) {
        QJsonObject hint = val.toObject();
        QString type = hint["type"].toString();
        QString inputHint = hint["input_hint"].toString();
        QString outputHint = hint["output_hint"].toString();
        
        if (!inputHint.isEmpty() && !outputHint.isEmpty()) {
            // 清理AI生成的测试用例文本
            QString cleanedInput = cleanTestCaseText(inputHint);
            QString cleanedOutput = cleanTestCaseText(outputHint);
            
            // 只有清理后的文本有效才添加
            if (!cleanedInput.isEmpty() && !cleanedOutput.isEmpty()) {
                TestCase tc;
                tc.input = cleanedInput;
                tc.expectedOutput = cleanedOutput;
                tc.description = type;
                tc.isAIGenerated = true;
                testCases.append(tc);
                
                qDebug() << "[SmartQuestionImporter] 添加AI生成测试用例:" << type;
            } else {
                qDebug() << "[SmartQuestionImporter] AI测试用例无效，已跳过:" << type;
            }
        }
    }
    
    // 不再自动添加占位符测试用例
    // 如果测试用例不足，保持实际数量即可
    if (testCases.size() < 3) {
        qDebug() << "[SmartQuestionImporter] 题目测试用例不足3个（当前" << testCases.size() << "个）";
    }
    
    return testCases;
}

// ==================== 进度计算方法 ====================

int ImportProgress::calculatePercentage() const
{
    switch (currentStage) {
        case Scanning: {
            // 扫描阶段: 0% → 10%
            if (totalFiles == 0) return 0;
            return (processedFiles * 10) / totalFiles;
        }
            
        case Parsing: {
            // AI解析阶段: 10% → 95%
            if (totalFiles == 0) return 10;
            
            // 基础进度：已完成文件的进度
            int baseProgress = 10 + (currentFileIndex * 85) / totalFiles;
            
            // 当前文件内的进度：基于已识别题目数
            // 使用对数函数平滑增长：y = 1 - e^(-x/5)
            // 1道题→18%, 3道题→45%, 5道题→63%, 10道题→86%, 15+道题→95%
            double currentFileBonus = 0;
            if (totalQuestions > 0 && totalFiles > 0) {
                double factor = totalQuestions / 5.0;
                int progressPerFile = 85 / totalFiles;
                currentFileBonus = progressPerFile * (1 - exp(-factor));
            }
            
            int result = baseProgress + static_cast<int>(currentFileBonus);
            return qMin(95, qMax(10, result));  // 限制在10-95%之间
        }
            
        case Saving:
            // 保存阶段: 95% → 100%
            return 95 + (saveProgress * 5) / 100;
            
        case Complete:
            return 100;
            
        default:
            return 0;
    }
}

// ==================== 阶段转换方法 ====================

void SmartQuestionImporter::enterScanningStage()
{
    m_progress.currentStage = ImportProgress::Scanning;
    m_progress.currentStatus = "扫描文件";
    updateProgress();
}

void SmartQuestionImporter::enterParsingStage()
{
    m_progress.currentStage = ImportProgress::Parsing;
    m_progress.currentStatus = "AI解析题目";
    updateProgress();
}

void SmartQuestionImporter::enterSavingStage()
{
    m_progress.currentStage = ImportProgress::Saving;
    m_progress.saveProgress = 0;
    m_progress.currentStatus = "保存完成";
    updateProgress();
}

void SmartQuestionImporter::enterCompleteStage()
{
    m_progress.currentStage = ImportProgress::Complete;
    m_progress.saveProgress = 100;
    m_progress.currentStatus = "导入完成";
    updateProgress();
}

void SmartQuestionImporter::updateProgress()
{
    emit progressUpdated(m_progress);
}

// ==================== 导入结果构建 ====================

ImportResult SmartQuestionImporter::buildImportResult(bool success, const QString &errorMessage)
{
    ImportResult result;
    result.success = success;
    result.totalQuestions = m_questions.size();
    result.basePath = QString("data/基础题库/%1/").arg(m_bankName);
    result.errorMessage = errorMessage;
    
    if (success) {
        // 按源文件统计（从chunks中获取文件名）
        QMap<QString, int> fileQuestionCount;
        for (const FileChunk &chunk : m_chunks) {
            fileQuestionCount[chunk.fileName] = 0;
        }
        
        // 简单统计：平均分配题目到文件
        if (!m_chunks.isEmpty()) {
            int questionsPerFile = m_questions.size() / m_chunks.size();
            int remainder = m_questions.size() % m_chunks.size();
            
            for (const FileChunk &chunk : m_chunks) {
                int count = questionsPerFile;
                if (remainder > 0) {
                    count++;
                    remainder--;
                }
                if (count > 0) {
                    result.questionsByFile[chunk.fileName] = count;
                }
            }
        }
        
        // 按难度统计
        for (const Question &q : m_questions) {
            QString difficultyStr;
            switch (q.difficulty()) {
                case Difficulty::Easy:
                    difficultyStr = "简单";
                    break;
                case Difficulty::Medium:
                    difficultyStr = "中等";
                    break;
                case Difficulty::Hard:
                    difficultyStr = "困难";
                    break;
                default:
                    difficultyStr = "未知";
                    break;
            }
            result.questionsByDifficulty[difficultyStr]++;
        }
    }
    
    return result;
}
