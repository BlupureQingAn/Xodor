#include "BatchTestCaseFixerDialog.h"
#include "../ai/OllamaClient.h"
#include "../core/QuestionBank.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDir>
#include <QRegularExpression>
#include <QApplication>

BatchTestCaseFixerDialog::BatchTestCaseFixerDialog(QuestionBank *questionBank, 
                                                   OllamaClient *aiClient, QWidget *parent)
    : QDialog(parent)
    , m_questionBank(questionBank)
    , m_aiClient(aiClient)
    , m_currentIndex(0)
    , m_isFixing(false)
{
    setupUI();
}

void BatchTestCaseFixerDialog::setupUI()
{
    setWindowTitle("批量测试用例修复工具");
    setMinimumSize(1000, 700);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // 标题
    QLabel *titleLabel = new QLabel("批量修复测试用例", this);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(14);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    mainLayout->addWidget(titleLabel);
    
    // 状态标签
    m_statusLabel = new QLabel("就绪", this);
    m_statusLabel->setStyleSheet("color: #888; padding: 5px;");
    mainLayout->addWidget(m_statusLabel);
    
    // 进度条
    m_progressBar = new QProgressBar(this);
    m_progressBar->setVisible(false);
    mainLayout->addWidget(m_progressBar);
    
    // 题目列表
    QLabel *listLabel = new QLabel("需要修复的题目：", this);
    mainLayout->addWidget(listLabel);
    
    m_questionList = new QListWidget(this);
    m_questionList->setMinimumHeight(200);
    m_questionList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    mainLayout->addWidget(m_questionList);
    
    // 按钮行
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    
    m_scanButton = new QPushButton("🔍 扫描问题", this);
    m_startButton = new QPushButton("🚀 开始批量修复", this);
    m_stopButton = new QPushButton("⏹ 停止", this);
    m_startButton->setEnabled(false);
    m_stopButton->setEnabled(false);
    
    buttonLayout->addWidget(m_scanButton);
    buttonLayout->addWidget(m_startButton);
    buttonLayout->addWidget(m_stopButton);
    buttonLayout->addStretch();
    
    QPushButton *closeButton = new QPushButton("关闭", this);
    buttonLayout->addWidget(closeButton);
    
    mainLayout->addLayout(buttonLayout);
    
    // 日志显示区
    QLabel *logLabel = new QLabel("修复日志：", this);
    mainLayout->addWidget(logLabel);
    
    m_logView = new QTextEdit(this);
    m_logView->setReadOnly(true);
    m_logView->setMinimumHeight(250);
    mainLayout->addWidget(m_logView);
    
    // 连接信号
    connect(m_scanButton, &QPushButton::clicked, this, &BatchTestCaseFixerDialog::onScanQuestions);
    connect(m_startButton, &QPushButton::clicked, this, &BatchTestCaseFixerDialog::onStartBatchFix);
    connect(m_stopButton, &QPushButton::clicked, this, &BatchTestCaseFixerDialog::onStopBatchFix);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
}

void BatchTestCaseFixerDialog::onScanQuestions()
{
    m_statusLabel->setText("状态：正在扫描题库...");
    m_statusLabel->setStyleSheet("color: blue; padding: 5px;");
    m_scanButton->setEnabled(false);
    
    scanAllQuestions();
    
    if (m_questionsToFix.isEmpty()) {
        m_statusLabel->setText("状态：未发现需要修复的题目");
        m_statusLabel->setStyleSheet("color: green; padding: 5px;");
        m_logView->append("✅ 扫描完成：所有题目的测试用例都正常！");
    } else {
        m_statusLabel->setText(QString("状态：发现 %1 个题目需要修复").arg(m_questionsToFix.size()));
        m_statusLabel->setStyleSheet("color: orange; padding: 5px;");
        m_startButton->setEnabled(true);
        m_logView->append(QString("📋 扫描完成：发现 %1 个题目需要修复\n").arg(m_questionsToFix.size()));
    }
    
    m_scanButton->setEnabled(true);
}

void BatchTestCaseFixerDialog::scanAllQuestions()
{
    m_questionsToFix.clear();
    m_questionList->clear();
    
    QDir dataDir("data/questions");
    if (!dataDir.exists()) {
        m_logView->append("❌ 错误：题库目录不存在");
        return;
    }
    
    QStringList jsonFiles = dataDir.entryList(QStringList() << "*.json", QDir::Files);
    int totalCount = jsonFiles.size();
    int scannedCount = 0;
    
    for (const QString &fileName : jsonFiles) {
        QString filePath = dataDir.filePath(fileName);
        
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly)) {
            continue;
        }
        
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        file.close();
        
        if (!doc.isObject()) {
            continue;
        }
        
        Question question(doc.object());
        
        // 检测是否有问题
        QVector<int> problematicIndices = detectProblematicTestCases(question);
        
        if (!problematicIndices.isEmpty()) {
            QuestionToFix qtf;
            qtf.id = question.id();
            qtf.title = question.title();
            qtf.filePath = filePath;
            qtf.problematicIndices = problematicIndices;
            
            m_questionsToFix.append(qtf);
            
            QString itemText = QString("%1. %2 (%3个问题)")
                .arg(m_questionsToFix.size())
                .arg(qtf.title)
                .arg(problematicIndices.size());
            m_questionList->addItem(itemText);
        }
        
        scannedCount++;
        m_logView->append(QString("扫描进度：%1/%2").arg(scannedCount).arg(totalCount));
        QApplication::processEvents();
    }
}

QVector<int> BatchTestCaseFixerDialog::detectProblematicTestCases(const Question &question)
{
    QVector<int> problematicIndices;
    QVector<TestCase> testCases = question.testCases();
    
    for (int i = 0; i < testCases.size(); ++i) {
        const TestCase &tc = testCases[i];
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
            problematicIndices.append(i);
        }
    }
    
    return problematicIndices;
}

void BatchTestCaseFixerDialog::onStartBatchFix()
{
    if (!m_aiClient) {
        QMessageBox::warning(this, "错误", "AI客户端未初始化");
        return;
    }
    
    if (m_questionsToFix.isEmpty()) {
        QMessageBox::information(this, "提示", "没有需要修复的题目");
        return;
    }
    
    m_isFixing = true;
    m_currentIndex = 0;
    m_startButton->setEnabled(false);
    m_stopButton->setEnabled(true);
    m_scanButton->setEnabled(false);
    
    m_progressBar->setMaximum(m_questionsToFix.size());
    m_progressBar->setValue(0);
    m_progressBar->setVisible(true);
    
    m_logView->append("\n========== 开始批量修复 ==========\n");
    
    fixNextQuestion();
}

void BatchTestCaseFixerDialog::fixNextQuestion()
{
    if (!m_isFixing || m_currentIndex >= m_questionsToFix.size()) {
        // 修复完成
        m_isFixing = false;
        m_startButton->setEnabled(true);
        m_stopButton->setEnabled(false);
        m_scanButton->setEnabled(true);
        m_statusLabel->setText("状态：批量修复完成");
        m_statusLabel->setStyleSheet("color: green; padding: 5px;");
        m_logView->append("\n========== 批量修复完成 ==========\n");
        
        // 发送信号通知主窗口重新加载
        emit batchFixCompleted();
        
        QMessageBox::information(this, "完成", "批量修复已完成！题库将自动重新加载。");
        return;
    }
    
    const QuestionToFix &qtf = m_questionsToFix[m_currentIndex];
    
    m_statusLabel->setText(QString("状态：正在修复 %1/%2 - %3")
        .arg(m_currentIndex + 1)
        .arg(m_questionsToFix.size())
        .arg(qtf.title));
    m_statusLabel->setStyleSheet("color: blue; padding: 5px;");
    
    m_logView->append(QString("\n[%1/%2] 正在修复：%3")
        .arg(m_currentIndex + 1)
        .arg(m_questionsToFix.size())
        .arg(qtf.title));
    
    // 加载题目
    QFile file(qtf.filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        m_logView->append("❌ 错误：无法打开文件");
        m_currentIndex++;
        fixNextQuestion();
        return;
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    
    m_currentQuestion = Question(doc.object());
    
    // 生成修复提示词
    QString prompt = generateFixPrompt(m_currentQuestion, qtf.problematicIndices);
    
    m_logView->append("📝 生成修复提示词...");
    m_logView->append("🤖 调用AI修复...");
    
    // 清空响应
    m_currentAIResponse.clear();
    
    // 连接AI信号
    connect(m_aiClient, &OllamaClient::streamingChunk, this, &BatchTestCaseFixerDialog::onAIChunk, Qt::UniqueConnection);
    connect(m_aiClient, &OllamaClient::streamingFinished, this, &BatchTestCaseFixerDialog::onAIFinished, Qt::UniqueConnection);
    connect(m_aiClient, &OllamaClient::error, this, &BatchTestCaseFixerDialog::onAIError, Qt::UniqueConnection);
    
    // 调用AI
    m_aiClient->sendChatMessage(prompt, "");
}

QString BatchTestCaseFixerDialog::generateFixPrompt(const Question &question, 
                                                    const QVector<int> &problematicIndices)
{
    QString prompt = QString(R"(你是一个测试用例修复专家。请修复以下C++编程题目的测试用例。

【题目信息】
标题：%1
描述：%2

【有问题的测试用例】
)").arg(question.title(), question.description());
    
    QVector<TestCase> testCases = question.testCases();
    for (int idx : problematicIndices) {
        if (idx < 0 || idx >= testCases.size()) continue;
        
        const TestCase &tc = testCases[idx];
        prompt += QString("\n测试用例 %1：\n").arg(idx + 1);
        prompt += QString("描述：%1\n").arg(tc.description);
        prompt += QString("当前输入：\n%1\n\n").arg(tc.input);
        prompt += QString("期望输出：\n%1\n\n").arg(tc.expectedOutput);
    }
    
    prompt += R"(
【修复要求】
1. 将所有省略号（...）和"重复"标记展开为完整的输入数据
2. 确保输入格式符合题目要求
3. 输入数据要完整、准确、可直接使用
4. 保持原有的测试用例描述和期望输出不变

【输出格式】
请以JSON格式输出修复后的测试用例：
```json
[
    {
        "index": 1,
        "description": "测试用例描述",
        "input": "完整输入",
        "output": "期望输出"
    }
]
```

请开始修复：)";
    
    return prompt;
}

void BatchTestCaseFixerDialog::onAIChunk(const QString &chunk)
{
    m_currentAIResponse += chunk;
}

void BatchTestCaseFixerDialog::onAIFinished()
{
    // 断开信号
    disconnect(m_aiClient, &OllamaClient::streamingChunk, this, &BatchTestCaseFixerDialog::onAIChunk);
    disconnect(m_aiClient, &OllamaClient::streamingFinished, this, &BatchTestCaseFixerDialog::onAIFinished);
    disconnect(m_aiClient, &OllamaClient::error, this, &BatchTestCaseFixerDialog::onAIError);
    
    m_logView->append("✅ AI响应完成");
    
    // 解析AI响应
    QRegularExpression jsonRegex(R"(```json\s*(\[[\s\S]*?\])\s*```)");
    QRegularExpressionMatch match = jsonRegex.match(m_currentAIResponse);
    
    if (!match.hasMatch()) {
        m_logView->append("❌ 错误：未找到有效的JSON格式");
        m_currentIndex++;
        fixNextQuestion();
        return;
    }
    
    QString jsonStr = match.captured(1);
    QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());
    
    if (!doc.isArray()) {
        m_logView->append("❌ 错误：JSON格式错误");
        m_currentIndex++;
        fixNextQuestion();
        return;
    }
    
    // 应用修复
    QVector<TestCase> testCases = m_currentQuestion.testCases();
    QJsonArray array = doc.array();
    int fixedCount = 0;
    
    for (const QJsonValue &val : array) {
        if (!val.isObject()) continue;
        
        QJsonObject obj = val.toObject();
        int index = obj["index"].toInt() - 1;
        
        if (index < 0 || index >= testCases.size()) continue;
        
        testCases[index].input = obj["input"].toString();
        testCases[index].expectedOutput = obj["output"].toString();
        fixedCount++;
    }
    
    m_currentQuestion.setTestCases(testCases);
    
    // 保存
    const QuestionToFix &qtf = m_questionsToFix[m_currentIndex];
    if (saveFixedQuestion(m_currentQuestion, qtf.filePath)) {
        m_logView->append(QString("💾 成功修复并保存 %1 个测试用例").arg(fixedCount));
    } else {
        m_logView->append("❌ 错误：保存失败");
    }
    
    // 更新进度
    m_progressBar->setValue(m_currentIndex + 1);
    
    // 继续下一个
    m_currentIndex++;
    fixNextQuestion();
}

void BatchTestCaseFixerDialog::onAIError(const QString &error)
{
    // 断开信号
    disconnect(m_aiClient, &OllamaClient::streamingChunk, this, &BatchTestCaseFixerDialog::onAIChunk);
    disconnect(m_aiClient, &OllamaClient::streamingFinished, this, &BatchTestCaseFixerDialog::onAIFinished);
    disconnect(m_aiClient, &OllamaClient::error, this, &BatchTestCaseFixerDialog::onAIError);
    
    m_logView->append(QString("❌ AI调用失败：%1").arg(error));
    
    // 继续下一个
    m_currentIndex++;
    fixNextQuestion();
}

void BatchTestCaseFixerDialog::onStopBatchFix()
{
    m_isFixing = false;
    m_statusLabel->setText("状态：已停止");
    m_statusLabel->setStyleSheet("color: orange; padding: 5px;");
    m_logView->append("\n⏹ 用户停止了批量修复\n");
    
    m_startButton->setEnabled(true);
    m_stopButton->setEnabled(false);
    m_scanButton->setEnabled(true);
}

bool BatchTestCaseFixerDialog::saveFixedQuestion(const Question &question, const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    
    QJsonDocument doc(question.toJson());
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    
    return true;
}
