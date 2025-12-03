#include "AIImportDialog.h"
#include "../ai/OllamaClient.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMessageBox>
#include <QTimer>

AIImportDialog::AIImportDialog(const QString &folderPath, OllamaClient *aiClient, QWidget *parent)
    : QDialog(parent)
    , m_folderPath(folderPath)
    , m_aiClient(aiClient)
    , m_success(false)
    , m_currentStep(0)
{
    setupUI();
    setWindowTitle("AI智能导入题库");
    resize(700, 500);
    
    // 自动开始导入
    QTimer::singleShot(500, this, &AIImportDialog::startImport);
}

void AIImportDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(16);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    
    // 标题
    QLabel *titleLabel = new QLabel("🤖 AI智能解析题库", this);
    titleLabel->setStyleSheet("font-size: 16pt; font-weight: bold; color: #e8e8e8;");
    
    // 状态标签
    m_statusLabel = new QLabel("准备扫描文件...", this);
    m_statusLabel->setStyleSheet("color: #b0b0b0; font-size: 10pt;");
    
    // 进度条
    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setStyleSheet(R"(
        QProgressBar {
            border: 1px solid #3a3a3a;
            border-radius: 8px;
            background-color: #242424;
            text-align: center;
            color: #e8e8e8;
            height: 30px;
        }
        QProgressBar::chunk {
            background-color: #660000;
            border-radius: 7px;
        }
    )");
    
    // 日志文本框
    QLabel *logLabel = new QLabel("📋 处理日志:", this);
    logLabel->setStyleSheet("color: #e8e8e8; font-weight: bold;");
    
    m_logText = new QTextEdit(this);
    m_logText->setReadOnly(true);
    m_logText->setStyleSheet(R"(
        QTextEdit {
            background-color: #1e1e1e;
            color: #e8e8e8;
            border: 1px solid #3a3a3a;
            border-radius: 8px;
            padding: 10px;
            font-family: 'Consolas', 'Monaco', monospace;
            font-size: 9pt;
        }
    )");
    
    // 按钮
    QHBoxLayout *btnLayout = new QHBoxLayout();
    m_cancelBtn = new QPushButton("取消", this);
    m_closeBtn = new QPushButton("完成", this);
    m_closeBtn->setEnabled(false);
    
    QString btnStyle = R"(
        QPushButton {
            background-color: #660000;
            color: white;
            border: none;
            border-radius: 8px;
            padding: 10px 24px;
            font-weight: 500;
            min-width: 80px;
        }
        QPushButton:hover {
            background-color: #880000;
        }
        QPushButton:pressed {
            background-color: #440000;
        }
        QPushButton:disabled {
            background-color: #3a3a3a;
            color: #707070;
        }
    )";
    
    m_cancelBtn->setStyleSheet(btnStyle);
    m_closeBtn->setStyleSheet(btnStyle);
    
    btnLayout->addStretch();
    btnLayout->addWidget(m_cancelBtn);
    btnLayout->addWidget(m_closeBtn);
    
    mainLayout->addWidget(titleLabel);
    mainLayout->addWidget(m_statusLabel);
    mainLayout->addWidget(m_progressBar);
    mainLayout->addWidget(logLabel);
    mainLayout->addWidget(m_logText);
    mainLayout->addLayout(btnLayout);
    
    // 连接信号
    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    
    // 应用对话框样式
    setStyleSheet(R"(
        QDialog {
            background-color: #242424;
        }
        QLabel {
            color: #e8e8e8;
        }
    )");
}

void AIImportDialog::startImport()
{
    m_logText->clear();
    m_logText->append("=== AI智能导入开始 ===\n\n");
    
    // 第一步：扫描文件
    m_statusLabel->setText("步骤 1/3: 扫描文件");
    m_progressBar->setValue(10);
    m_logText->append("[1/3] 📂 扫描目录中的文件...\n");
    scanFiles();
    
    // 第二步：发送给AI解析
    if (!m_fileContents.isEmpty()) {
        m_statusLabel->setText("步骤 2/3: AI解析题目");
        m_progressBar->setValue(30);
        m_logText->append("\n[2/3] 🤖 发送给AI进行智能解析...\n");
        sendToAI();
    } else {
        m_logText->append("\n❌ 错误：未找到任何Markdown文件\n");
        m_statusLabel->setText("❌ 导入失败：未找到文件");
        m_progressBar->setValue(0);
        m_cancelBtn->setEnabled(false);
        m_closeBtn->setEnabled(true);
    }
}

void AIImportDialog::scanFiles()
{
    QDir dir(m_folderPath);
    QStringList filters;
    filters << "*.md" << "*.markdown" << "*.txt";
    
    QFileInfoList files = dir.entryInfoList(filters, QDir::Files);
    
    m_logText->append(QString("  目录: %1\n").arg(m_folderPath));
    m_logText->append(QString("  找到 %1 个文件\n\n").arg(files.size()));
    
    int totalChars = 0;
    for (const QFileInfo &fileInfo : files) {
        QFile file(fileInfo.absoluteFilePath());
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            in.setEncoding(QStringConverter::Utf8);
            QString content = in.readAll();
            file.close();
            
            m_fileContents.append(content);
            m_fileNames.append(fileInfo.fileName());
            totalChars += content.length();
            
            m_logText->append(QString("  ✓ %1 (%2 字符)\n")
                .arg(fileInfo.fileName())
                .arg(content.length()));
        }
    }
    
    m_logText->append(QString("\n  总计: %1 个文件, %2 字符\n")
        .arg(files.size())
        .arg(totalChars));
}

void AIImportDialog::sendToAI()
{
    if (!m_aiClient) {
        m_logText->append("\n❌ 错误：AI客户端未初始化\n");
        m_statusLabel->setText("❌ 导入失败：AI未连接");
        m_progressBar->setValue(0);
        m_cancelBtn->setEnabled(false);
        m_closeBtn->setEnabled(true);
        return;
    }
    
    QString prompt = buildAIPrompt();
    
    m_logText->append(QString("  提示词长度: %1 字符\n").arg(prompt.length()));
    m_logText->append("  正在发送请求...\n");
    m_logText->append("  ⏳ AI正在分析，请稍候...\n");
    
    // 连接AI响应信号
    connect(m_aiClient, &OllamaClient::codeAnalysisReady, 
            this, &AIImportDialog::onAIResponse, Qt::UniqueConnection);
    connect(m_aiClient, &OllamaClient::error, 
            this, &AIImportDialog::onAIError, Qt::UniqueConnection);
    
    // 发送请求
    m_aiClient->analyzeCode("", prompt);
    
    m_progressBar->setValue(40);
}

QString AIImportDialog::buildAIPrompt()
{
    QString prompt = R"(
你是一个专业的编程题目解析和测试用例生成助手。我会给你一些Markdown格式的文件内容，请帮我提取其中的编程题目并生成完整的测试数据集。

任务要求：
1. 识别所有编程题目（忽略目录、说明、介绍等非题目内容）
2. 对每道题提取以下信息：
   - 标题（title）
   - 难度（difficulty）：简单/中等/困难
   - 描述（description）：题目的完整描述
   - 标签（tags）：如数组、字符串、动态规划等
   - 测试用例（testCases）：至少5组完整的测试数据

3. 测试用例生成要求（重要！）：
   ✓ 基本功能测试（2-3个）：验证核心功能
   ✓ 边界条件测试（1-2个）：空输入、最小值、最大值
   ✓ 特殊情况测试（1-2个）：负数、零、重复元素、无解等
   ✓ 每个测试用例必须包含：input（输入）、output（期望输出）

4. 难度判断标准：
   - 简单：基础语法、简单逻辑、单一数据结构
   - 中等：多个数据结构、算法应用、需要优化
   - 困难：复杂算法、多重优化、高级数据结构

5. 以JSON格式返回，格式如下：
```json
{
  "questions": [
    {
      "title": "两数之和",
      "difficulty": "简单",
      "description": "给定一个整数数组 nums 和一个整数目标值 target，请你在该数组中找出和为目标值的那两个整数，并返回它们的数组下标。",
      "tags": ["数组", "哈希表"],
      "testCases": [
        {
          "input": "[2,7,11,15]\n9",
          "output": "[0,1]"
        },
        {
          "input": "[3,2,4]\n6",
          "output": "[1,2]"
        },
        {
          "input": "[3,3]\n6",
          "output": "[0,1]"
        },
        {
          "input": "[]\n0",
          "output": "[]"
        },
        {
          "input": "[-1,-2,-3,-4,-5]\n-8",
          "output": "[2,4]"
        }
      ]
    }
  ]
}
```

注意事项：
- 只提取真正的编程题目，忽略目录、说明等
- 测试用例必须至少5组，覆盖不同情况
- 如果原文没有足够测试用例，请根据题目描述智能生成
- 测试用例的input和output格式要清晰，便于程序解析
- 返回纯JSON，不要有其他文字

以下是文件内容：
---
)";
    
    // 添加所有文件内容
    for (int i = 0; i < m_fileContents.size(); ++i) {
        prompt += QString("\n=== 文件 %1: %2 ===\n").arg(i + 1).arg(m_fileNames[i]);
        prompt += m_fileContents[i];
        prompt += "\n\n";
    }
    
    prompt += R"(
---

请开始解析，直接返回JSON格式的结果。
)";
    
    return prompt;
}

void AIImportDialog::onAIResponse(const QString &response)
{
    m_logText->append("\n  ✓ AI响应接收完成\n");
    m_logText->append(QString("  响应长度: %1 字符\n").arg(response.length()));
    
    m_statusLabel->setText("步骤 3/3: 解析结果");
    m_progressBar->setValue(70);
    
    m_logText->append("\n[3/3] 🔍 解析AI返回的题目数据...\n");
    
    parseAIResponse(response);
}

void AIImportDialog::parseAIResponse(const QString &response)
{
    // 提取JSON部分（AI可能返回额外的文字）
    QString jsonStr = response;
    
    m_logText->append("  正在提取JSON数据...\n");
    
    // 尝试找到JSON代码块
    int jsonStart = response.indexOf("```json");
    if (jsonStart >= 0) {
        jsonStart = response.indexOf('\n', jsonStart) + 1;
        int jsonEnd = response.indexOf("```", jsonStart);
        if (jsonEnd > jsonStart) {
            jsonStr = response.mid(jsonStart, jsonEnd - jsonStart).trimmed();
            m_logText->append("  ✓ 找到JSON代码块\n");
        }
    } else {
        // 尝试找到 { 开始的JSON
        jsonStart = response.indexOf('{');
        if (jsonStart >= 0) {
            jsonStr = response.mid(jsonStart);
            m_logText->append("  ✓ 找到JSON对象\n");
        }
    }
    
    m_progressBar->setValue(80);
    
    // 解析JSON
    QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());
    
    if (doc.isNull() || !doc.isObject()) {
        m_logText->append("\n❌ 错误：无法解析AI返回的JSON格式\n");
        m_logText->append("  可能原因：AI返回格式不正确或不完整\n");
        m_statusLabel->setText("❌ 解析失败：JSON格式错误");
        m_progressBar->setValue(0);
        m_cancelBtn->setEnabled(false);
        m_closeBtn->setEnabled(true);
        return;
    }
    
    QJsonObject root = doc.object();
    QJsonArray questionsArray = root["questions"].toArray();
    
    if (questionsArray.isEmpty()) {
        m_logText->append("\n❌ 错误：未找到任何题目\n");
        m_statusLabel->setText("❌ 解析失败：未找到题目");
        m_progressBar->setValue(0);
        m_cancelBtn->setEnabled(false);
        m_closeBtn->setEnabled(true);
        return;
    }
    
    m_logText->append(QString("  ✓ 成功解析 %1 道题目\n\n").arg(questionsArray.size()));
    m_progressBar->setValue(90);
    
    int successCount = 0;
    for (const QJsonValue &val : questionsArray) {
        QJsonObject qObj = val.toObject();
        
        QString title = qObj["title"].toString();
        if (title.isEmpty()) {
            m_logText->append("  ⚠ 跳过：题目标题为空\n");
            continue;
        }
        
        Question q;
        q.setId(QString("q_%1").arg(qHash(title)));
        q.setTitle(title);
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
            testCases.append(tc);
        }
        q.setTestCases(testCases);
        q.setType(QuestionType::Code);
        
        m_questions.append(q);
        successCount++;
        
        QString diffEmoji = (q.difficulty() == Difficulty::Easy) ? "🟢" : 
                           (q.difficulty() == Difficulty::Hard) ? "🔴" : "🟡";
        
        m_logText->append(QString("  %1 %2 [%3] - %4个测试用例\n")
            .arg(diffEmoji)
            .arg(title)
            .arg(diffStr)
            .arg(testCases.size()));
    }
    
    m_logText->append(QString("\n=== 导入完成 ===\n"));
    m_logText->append(QString("成功导入: %1 道题目\n").arg(successCount));
    
    m_statusLabel->setText(QString("✅ 成功导入 %1 道题目").arg(successCount));
    m_progressBar->setValue(100);
    
    m_success = true;
    m_cancelBtn->setEnabled(false);
    m_closeBtn->setEnabled(true);
    
    emit importFinished(true);
}

void AIImportDialog::onAIError(const QString &error)
{
    m_logText->append(QString("\n❌ AI错误: %1\n").arg(error));
    m_logText->append("\n请检查：\n");
    m_logText->append("  1. Ollama服务是否正在运行\n");
    m_logText->append("  2. AI模型是否已下载\n");
    m_logText->append("  3. 网络连接是否正常\n");
    
    m_statusLabel->setText("❌ AI解析失败");
    m_progressBar->setValue(0);
    
    m_cancelBtn->setEnabled(false);
    m_closeBtn->setEnabled(true);
    
    QMessageBox::critical(this, "AI解析失败", 
        QString("AI解析过程中出现错误：\n\n%1\n\n请检查：\n"
                "1. Ollama服务是否正在运行\n"
                "2. 模型是否已下载\n"
                "3. 网络连接是否正常").arg(error));
    
    emit importFinished(false);
}
