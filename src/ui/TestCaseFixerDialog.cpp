#include "TestCaseFixerDialog.h"
#include "../ai/OllamaClient.h"
#include "../core/QuestionBank.h"
#include "../core/QuestionBankManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDir>
#include <QDirIterator>
#include <QRegularExpression>
#include <QApplication>

TestCaseFixerDialog::TestCaseFixerDialog(QuestionBank *questionBank, OllamaClient *aiClient,
                                         QWidget *parent)
    : QDialog(parent)
    , m_questionBank(questionBank)
    , m_aiClient(aiClient)
    , m_privateClient(nullptr)
    , m_currentScanIndex(0)
    , m_currentFixIndex(0)
    , m_isScanning(false)
    , m_isFixing(false)
    , m_currentMode(Idle)
{
    // 创建私有的 AI 客户端，避免影响主界面的对话框
    m_privateClient = new OllamaClient(this);
    
    // 从传入的客户端复制配置
    if (m_aiClient) {
        m_privateClient->setBaseUrl(m_aiClient->baseUrl());
        m_privateClient->setModel(m_aiClient->model());
        m_privateClient->setApiKey(m_aiClient->apiKey());
        m_privateClient->setCloudMode(m_aiClient->isCloudMode());
    }
    
    setupUI();
    loadAllQuestions();
}

void TestCaseFixerDialog::setupUI()
{
    setWindowTitle("测试用例修复工具");
    setMinimumSize(1100, 750);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // 标题
    QLabel *titleLabel = new QLabel("🔧 测试用例修复工具", this);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(14);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    mainLayout->addWidget(titleLabel);
    
    // 说明文字
    QLabel *descLabel = new QLabel("选择要检查的题目，点击'扫描问题'检测测试用例问题，然后修复有问题的题目。", this);
    descLabel->setStyleSheet("color: #666; padding: 5px;");
    mainLayout->addWidget(descLabel);
    
    // 状态和选择信息
    QHBoxLayout *infoLayout = new QHBoxLayout();
    m_statusLabel = new QLabel("就绪", this);
    m_statusLabel->setStyleSheet("color: #888; padding: 5px; font-size: 12px;");
    infoLayout->addWidget(m_statusLabel);
    
    m_selectionLabel = new QLabel("已选择: 0 个题目", this);
    m_selectionLabel->setStyleSheet("color: #666; padding: 5px; font-size: 12px;");
    infoLayout->addStretch();
    infoLayout->addWidget(m_selectionLabel);
    
    mainLayout->addLayout(infoLayout);
    
    // 进度条
    m_progressBar = new QProgressBar(this);
    m_progressBar->setVisible(false);
    m_progressBar->setTextVisible(true);
    m_progressBar->setFormat("%v/%m - %p%");
    mainLayout->addWidget(m_progressBar);
    
    // 内容区域
    QHBoxLayout *contentLayout = new QHBoxLayout();
    
    // 左侧：题目列表
    QVBoxLayout *leftLayout = new QVBoxLayout();
    
    QHBoxLayout *listHeaderLayout = new QHBoxLayout();
    QLabel *listLabel = new QLabel("题目列表：", this);
    listHeaderLayout->addWidget(listLabel);
    listHeaderLayout->addStretch();
    
    m_selectAllButton = new QPushButton("全选", this);
    m_selectAllButton->setMaximumWidth(60);
    m_selectNoneButton = new QPushButton("取消", this);
    m_selectNoneButton->setMaximumWidth(60);
    listHeaderLayout->addWidget(m_selectAllButton);
    listHeaderLayout->addWidget(m_selectNoneButton);
    
    leftLayout->addLayout(listHeaderLayout);
    
    m_questionList = new QListWidget(this);
    m_questionList->setMinimumWidth(350);
    m_questionList->setMaximumWidth(450);
    m_questionList->setSelectionMode(QAbstractItemView::NoSelection);  // 使用复选框而不是选择
    leftLayout->addWidget(m_questionList);
    
    contentLayout->addLayout(leftLayout);
    
    // 右侧：详细信息
    QVBoxLayout *rightLayout = new QVBoxLayout();
    QLabel *detailLabel = new QLabel("详细信息：", this);
    rightLayout->addWidget(detailLabel);
    
    m_detailView = new QTextEdit(this);
    m_detailView->setReadOnly(true);
    m_detailView->setLineWrapMode(QTextEdit::WidgetWidth);
    m_detailView->setPlaceholderText("选择题目后，点击'扫描问题'查看检测结果...");
    rightLayout->addWidget(m_detailView);
    
    contentLayout->addLayout(rightLayout, 1);
    
    mainLayout->addLayout(contentLayout);
    
    // 按钮行
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    
    m_scanButton = new QPushButton("🔍 扫描问题", this);
    m_scanButton->setMinimumHeight(35);
    m_scanButton->setToolTip("检测选中题目的测试用例问题");
    
    m_fixButton = new QPushButton("🚀 修复选中", this);
    m_fixButton->setMinimumHeight(35);
    m_fixButton->setEnabled(false);
    m_fixButton->setToolTip("修复有问题的题目");
    
    m_stopButton = new QPushButton("⏹ 停止", this);
    m_stopButton->setMinimumHeight(35);
    m_stopButton->setEnabled(false);
    
    buttonLayout->addWidget(m_scanButton);
    buttonLayout->addWidget(m_fixButton);
    buttonLayout->addWidget(m_stopButton);
    buttonLayout->addStretch();
    
    QPushButton *closeButton = new QPushButton("关闭", this);
    closeButton->setMinimumHeight(35);
    buttonLayout->addWidget(closeButton);
    
    mainLayout->addLayout(buttonLayout);
    
    // 日志显示区
    QLabel *logLabel = new QLabel("处理日志：", this);
    mainLayout->addWidget(logLabel);
    
    m_logView = new QTextEdit(this);
    m_logView->setReadOnly(true);
    m_logView->setMinimumHeight(120);
    m_logView->setMaximumHeight(150);
    m_logView->setPlaceholderText("等待操作...");
    mainLayout->addWidget(m_logView);
    
    // 连接信号
    connect(m_scanButton, &QPushButton::clicked, this, &TestCaseFixerDialog::onScanSelected);
    connect(m_selectAllButton, &QPushButton::clicked, this, &TestCaseFixerDialog::onSelectAll);
    connect(m_selectNoneButton, &QPushButton::clicked, this, &TestCaseFixerDialog::onSelectNone);
    connect(m_fixButton, &QPushButton::clicked, this, &TestCaseFixerDialog::onFixSelected);
    connect(m_stopButton, &QPushButton::clicked, this, &TestCaseFixerDialog::onStopFix);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(m_questionList, &QListWidget::itemChanged, this, &TestCaseFixerDialog::onQuestionItemChanged);
}

QString TestCaseFixerDialog::findValidBankPath()
{
    // 题库路径固定为：data/基础题库/当前题库名称/
    QString currentBankId = QuestionBankManager::instance().getCurrentBankId();
    
    if (!currentBankId.isEmpty()) {
        QuestionBankInfo bankInfo = QuestionBankManager::instance().getBankInfo(currentBankId);
        
        // 使用题库名称构建路径
        if (!bankInfo.name.isEmpty()) {
            QString bankPath = QString("data/基础题库/%1").arg(bankInfo.name);
            QDir dir(bankPath);
            if (dir.exists()) {
                return bankPath;
            }
        }
    }
    
    // 默认返回 data/基础题库
    return "data/基础题库";
}

void TestCaseFixerDialog::loadAllQuestions()
{
    m_allQuestions.clear();
    m_questionList->clear();
    
    if (!m_questionBank) {
        m_logView->append("❌ 错误：题库未初始化");
        return;
    }
    
    m_statusLabel->setText("📂 正在加载题目...");
    m_statusLabel->setStyleSheet("color: blue; padding: 5px; font-size: 12px;");
    
    // 从 QuestionBank 加载题目（与题库列表一样）
    QVector<Question> allQuestions = m_questionBank->allQuestions();
    
    if (allQuestions.isEmpty()) {
        m_statusLabel->setText("⚠️ 题库为空");
        m_statusLabel->setStyleSheet("color: orange; padding: 5px; font-size: 12px;");
        m_logView->append("⚠️ 题库为空，请先导入题目。");
        return;
    }
    
    // 智能获取题库路径
    QString bankPath = findValidBankPath();
    
    QDir bankDir(bankPath);
    
    // 递归扫描题库目录，建立题目标题和ID到文件路径的映射
    QMap<QString, QString> titleToPathMap;
    QMap<QString, QString> idToPathMap;
    
    // 使用递归迭代器扫描所有子目录
    QDirIterator it(bankPath, QStringList() << "*.json", QDir::Files, QDirIterator::Subdirectories);
    
    while (it.hasNext()) {
        QString filePath = it.next();
        
        // 读取文件获取标题和ID
        QFile file(filePath);
        if (file.open(QIODevice::ReadOnly)) {
            QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
            file.close();
            
            if (doc.isObject()) {
                QJsonObject obj = doc.object();
                QString title = obj["title"].toString();
                QString id = obj["id"].toString();
                
                if (!title.isEmpty()) {
                    titleToPathMap[title] = filePath;
                }
                if (!id.isEmpty()) {
                    idToPathMap[id] = filePath;
                }
            }
        }
    }
    
    // 使用 QuestionBank 中的题目，匹配文件路径
    int foundFiles = 0;
    int missingFiles = 0;
    
    for (const Question &question : allQuestions) {
        QuestionItem item;
        item.id = question.id();
        item.title = question.title();
        
        // 优先通过标题匹配文件路径
        if (titleToPathMap.contains(question.title())) {
            item.filePath = titleToPathMap[question.title()];
        }
        // 其次通过ID匹配
        else if (idToPathMap.contains(question.id())) {
            item.filePath = idToPathMap[question.id()];
        }
        // 最后尝试构建路径
        else {
            item.filePath = bankDir.absoluteFilePath(question.id() + ".json");
        }
        
        bool fileFound = QFile::exists(item.filePath);
        if (fileFound) {
            foundFiles++;
        } else {
            missingFiles++;
        }
        
        item.hasIssues = false;
        
        m_allQuestions.append(item);
        
        // 添加到列表（带复选框）
        QString displayTitle = question.title();
        if (!fileFound) {
            displayTitle = QString("⚠️ %1 (文件缺失)").arg(question.title());
        }
        QListWidgetItem *listItem = new QListWidgetItem(displayTitle, m_questionList);
        listItem->setFlags(listItem->flags() | Qt::ItemIsUserCheckable);
        listItem->setCheckState(Qt::Unchecked);
        if (!fileFound) {
            listItem->setForeground(QColor("#ff6b6b"));
        }
        
        QApplication::processEvents();
    }
    
    m_statusLabel->setText(QString("✅ 已加载 %1 个题目").arg(m_allQuestions.size()));
    m_statusLabel->setStyleSheet("color: green; padding: 5px; font-size: 12px;");
    
    QString logMsg = QString("📂 已加载 %1 个题目（来自：%2）\n").arg(m_allQuestions.size()).arg(bankPath);
    logMsg += QString("   找到文件：%1 个\n").arg(foundFiles);
    if (missingFiles > 0) {
        logMsg += QString("   ⚠️ 缺失文件：%1 个").arg(missingFiles);
    }
    m_logView->append(logMsg);
}

void TestCaseFixerDialog::onSelectAll()
{
    for (int i = 0; i < m_questionList->count(); ++i) {
        m_questionList->item(i)->setCheckState(Qt::Checked);
    }
}

void TestCaseFixerDialog::onSelectNone()
{
    for (int i = 0; i < m_questionList->count(); ++i) {
        m_questionList->item(i)->setCheckState(Qt::Unchecked);
    }
}

void TestCaseFixerDialog::onQuestionItemChanged(QListWidgetItem *item)
{
    updateStatusLabel();
}

void TestCaseFixerDialog::updateStatusLabel()
{
    int selectedCount = 0;
    for (int i = 0; i < m_questionList->count(); ++i) {
        if (m_questionList->item(i)->checkState() == Qt::Checked) {
            selectedCount++;
        }
    }
    m_selectionLabel->setText(QString("已选择: %1 个题目").arg(selectedCount));
}

void TestCaseFixerDialog::onScanSelected()
{
    if (!m_privateClient) {
        QMessageBox::warning(this, "错误", "AI客户端未初始化");
        return;
    }
    
    // 获取选中的题目
    m_selectedIndices.clear();
    for (int i = 0; i < m_questionList->count(); ++i) {
        if (m_questionList->item(i)->checkState() == Qt::Checked) {
            m_selectedIndices.append(i);
        }
    }
    
    if (m_selectedIndices.isEmpty()) {
        QMessageBox::information(this, "提示", "请先选择要检查的题目");
        return;
    }
    
    m_isScanning = true;
    m_currentMode = Scanning;
    m_currentScanIndex = 0;
    m_questionsToFix.clear();
    
    m_scanButton->setEnabled(false);
    m_fixButton->setEnabled(false);
    m_stopButton->setEnabled(true);
    m_progressBar->setVisible(true);
    m_progressBar->setMaximum(m_selectedIndices.size());
    m_progressBar->setValue(0);
    
    m_statusLabel->setText("🔍 正在扫描...");
    m_statusLabel->setStyleSheet("color: blue; padding: 5px; font-size: 12px;");
    m_logView->clear();
    m_logView->append(QString("🔍 开始AI扫描 %1 个题目...\n").arg(m_selectedIndices.size()));
    m_detailView->clear();
    
    scanNextQuestion();
}

void TestCaseFixerDialog::scanNextQuestion()
{
    if (!m_isScanning || m_currentScanIndex >= m_selectedIndices.size()) {
        // 扫描完成
        m_isScanning = false;
        m_currentMode = Idle;
        m_scanButton->setEnabled(true);
        m_stopButton->setEnabled(false);
        m_progressBar->setVisible(false);
        
        // 显示扫描结果
        int issueCount = m_questionsToFix.size();
        QString summary = QString("\n========== 扫描完成 ==========\n");
        summary += QString("扫描题目：%1 个\n").arg(m_selectedIndices.size());
        summary += QString("发现问题：%1 个\n").arg(issueCount);
        summary += QString("正常题目：%1 个\n").arg(m_selectedIndices.size() - issueCount);
        m_logView->append(summary);
        
        if (issueCount > 0) {
            m_statusLabel->setText(QString("⚠️ 发现 %1 个题目存在问题").arg(issueCount));
            m_statusLabel->setStyleSheet("color: orange; padding: 5px; font-size: 12px;");
            m_fixButton->setEnabled(true);
            
            // 显示详细信息
            QString detail = QString("AI分析发现 %1 个题目存在测试用例问题：\n\n").arg(issueCount);
            for (const QuestionItem &item : m_questionsToFix) {
                detail += QString("⚠️ %1\n").arg(item.title);
                detail += QString("   问题数量：%1 个\n\n").arg(item.problematicIndices.size());
            }
            detail += "\n点击'修复选中'按钮开始修复。";
            m_detailView->setPlainText(detail);
        } else {
            m_statusLabel->setText("✅ 所有题目正常");
            m_statusLabel->setStyleSheet("color: green; padding: 5px; font-size: 12px;");
            m_fixButton->setEnabled(false);
            m_detailView->setPlainText("✅ AI分析：所有选中的题目测试用例都正常，无需修复。");
        }
        return;
    }
    
    int idx = m_selectedIndices[m_currentScanIndex];
    if (idx < 0 || idx >= m_allQuestions.size()) {
        m_currentScanIndex++;
        scanNextQuestion();
        return;
    }
    
    QuestionItem &item = m_allQuestions[idx];
    
    m_statusLabel->setText(QString("🔍 正在扫描 %1/%2 - %3")
        .arg(m_currentScanIndex + 1)
        .arg(m_selectedIndices.size())
        .arg(item.title));
    
    m_logView->append(QString("[%1/%2] 正在AI分析：%3")
        .arg(m_currentScanIndex + 1)
        .arg(m_selectedIndices.size())
        .arg(item.title));
    
    // 加载题目
    QFile file(item.filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        m_logView->append(QString("❌ 错误：无法打开文件 - %1").arg(item.filePath));
        
        // 更新列表项显示为错误
        QListWidgetItem *listItem = m_questionList->item(idx);
        if (listItem) {
            listItem->setText(QString("❌ %1 (文件不存在)").arg(item.title));
            listItem->setForeground(QColor("#ff0000"));
        }
        
        m_progressBar->setValue(m_currentScanIndex + 1);
        m_currentScanIndex++;
        scanNextQuestion();
        return;
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    
    m_currentQuestion = Question(doc.object());
    
    // 生成扫描提示词
    QString prompt = generateScanPrompt(m_currentQuestion);
    m_currentAIResponse.clear();
    
    // 连接AI信号（使用私有客户端，不影响主界面）
    connect(m_privateClient, &OllamaClient::streamingChunk, this, &TestCaseFixerDialog::onAIChunk, Qt::UniqueConnection);
    connect(m_privateClient, &OllamaClient::streamingFinished, this, &TestCaseFixerDialog::onAIFinished, Qt::UniqueConnection);
    connect(m_privateClient, &OllamaClient::error, this, &TestCaseFixerDialog::onAIError, Qt::UniqueConnection);
    
    // 调用AI
    m_privateClient->sendChatMessage(prompt, "");
}

QString TestCaseFixerDialog::generateScanPrompt(const Question &question)
{
    QString prompt = QString(R"(你是一个测试用例质量检查专家。请分析以下C++编程题目的测试用例，判断是否存在问题。

【题目信息】
标题：%1
描述：%2

【测试用例】
)").arg(question.title(), question.description());
    
    QVector<TestCase> testCases = question.testCases();
    for (int i = 0; i < testCases.size(); ++i) {
        const TestCase &tc = testCases[i];
        prompt += QString("\n测试用例 %1：\n").arg(i + 1);
        prompt += QString("描述：%1\n").arg(tc.description);
        prompt += QString("输入：\n%1\n\n").arg(tc.input.left(300));
        prompt += QString("期望输出：\n%1\n\n").arg(tc.expectedOutput.left(300));
    }
    
    prompt += R"(
【重点检查】
⚠️ 特别注意：用文字或符号代替实际数据的情况！

常见问题：
1. 省略号：如"..."、"...（重复100次）"、"...重复"
2. 文字描述：如"（此处省略98行）"、"（重复n次）"、"（数据过长省略）"
3. 符号代替：如"[...]"、"<省略>"、"..."
4. 不完整数据：描述说100行，实际只有几行
5. 格式问题：输入输出格式不符合题目要求

【检查要点】
1. 输入数据必须是完整的、可直接复制使用的实际数据
2. 不能包含任何文字描述或符号代替
3. 数据行数必须与描述匹配
4. 期望输出必须完整
5. 格式必须符合题目要求

【输出格式】
请以JSON格式输出分析结果：
```json
{
    "hasIssues": true/false,
    "problematicIndices": [1, 3, 5],
    "summary": "简要说明发现的问题"
}
```

如果没有问题，返回：
```json
{
    "hasIssues": false,
    "problematicIndices": [],
    "summary": "所有测试用例都是完整的实际数据"
}
```

请开始分析：)";
    
    return prompt;
}

void TestCaseFixerDialog::onFixSelected()
{
    if (!m_privateClient) {
        QMessageBox::warning(this, "错误", "AI客户端未初始化");
        return;
    }
    
    if (m_questionsToFix.isEmpty()) {
        QMessageBox::information(this, "提示", "没有需要修复的题目");
        return;
    }
    
    m_isFixing = true;
    m_currentMode = Fixing;
    m_currentFixIndex = 0;
    m_scanButton->setEnabled(false);
    m_fixButton->setEnabled(false);
    m_stopButton->setEnabled(true);
    m_progressBar->setVisible(true);
    m_progressBar->setMaximum(m_questionsToFix.size());
    m_progressBar->setValue(0);
    
    m_logView->clear();
    m_logView->append("========== 开始AI修复 ==========\n");
    
    fixNextQuestion();
}

void TestCaseFixerDialog::fixNextQuestion()
{
    if (!m_isFixing || m_currentFixIndex >= m_questionsToFix.size()) {
        // 修复完成
        m_isFixing = false;
        m_currentMode = Idle;
        m_scanButton->setEnabled(true);
        m_fixButton->setEnabled(false);
        m_stopButton->setEnabled(false);
        m_progressBar->setVisible(false);
        m_statusLabel->setText("✅ 修复完成");
        m_statusLabel->setStyleSheet("color: green; padding: 5px; font-size: 12px;");
        m_logView->append("\n========== 修复完成 ==========\n");
        
        // 发送信号通知主窗口刷新
        emit questionsFixed();
        
        QMessageBox::information(this, "完成", 
            QString("AI修复完成！\n\n共修复 %1 个题目。\n题库将自动刷新。").arg(m_questionsToFix.size()));
        return;
    }
    
    const QuestionItem &item = m_questionsToFix[m_currentFixIndex];
    
    m_statusLabel->setText(QString("🚀 正在修复 %1/%2 - %3")
        .arg(m_currentFixIndex + 1)
        .arg(m_questionsToFix.size())
        .arg(item.title));
    m_statusLabel->setStyleSheet("color: blue; padding: 5px; font-size: 12px;");
    
    m_logView->append(QString("\n[%1/%2] 正在AI修复：%3")
        .arg(m_currentFixIndex + 1)
        .arg(m_questionsToFix.size())
        .arg(item.title));
    
    // 加载题目
    QFile file(item.filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        m_logView->append("❌ 错误：无法打开文件");
        m_currentFixIndex++;
        fixNextQuestion();
        return;
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    
    m_currentQuestion = Question(doc.object());
    
    // 生成修复提示词
    QString prompt = generateFixPrompt(m_currentQuestion, item.problematicIndices);
    m_currentAIResponse.clear();
    
    // 连接AI信号（使用私有客户端，不影响主界面）
    connect(m_privateClient, &OllamaClient::streamingChunk, this, &TestCaseFixerDialog::onAIChunk, Qt::UniqueConnection);
    connect(m_privateClient, &OllamaClient::streamingFinished, this, &TestCaseFixerDialog::onAIFinished, Qt::UniqueConnection);
    connect(m_privateClient, &OllamaClient::error, this, &TestCaseFixerDialog::onAIError, Qt::UniqueConnection);
    
    // 调用AI
    m_privateClient->sendChatMessage(prompt, "");
}

QString TestCaseFixerDialog::generateFixPrompt(const Question &question, 
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
【修复要求 - 非常重要！】
⚠️ 必须生成完整的、可直接使用的实际数据！

1. 将所有省略号（...）、"重复"标记、文字描述展开为完整的实际数据
2. 不能使用任何文字描述或符号代替数据
3. 数据必须完整、准确、可直接复制使用
4. 如果描述说100行，必须生成完整的100行数据
5. 输入格式必须严格符合题目要求
6. 期望输出也必须是完整的实际数据

【错误示例】
❌ 错误：
input: "100 1\n1 0\n...（重复98行）\n0 0"

✅ 正确：
input: "100 1\n1 0\n1 0\n1 0\n...(完整的100行)\n0 0"

【输出格式】
请以JSON格式输出修复后的测试用例：
```json
[
    {
        "index": 1,
        "description": "测试用例描述",
        "input": "完整的实际数据（不能有省略号或文字描述）",
        "output": "完整的期望输出"
    }
]
```

⚠️ 重要提醒：
- input和output字段必须包含完整的实际数据
- 不能包含"..."、"（重复）"、"省略"等任何文字或符号
- 数据必须可以直接用于程序测试

请开始修复：)";
    
    return prompt;
}

void TestCaseFixerDialog::onAIChunk(const QString &chunk)
{
    m_currentAIResponse += chunk;
    
    // 简洁的进度显示
    static int dotCount = 0;
    dotCount = (dotCount + 1) % 4;
    QString dots = QString(".").repeated(dotCount);
    m_statusLabel->setText(QString("🤖 AI正在处理%1").arg(dots));
}

void TestCaseFixerDialog::onAIFinished()
{
    // 断开信号（使用私有客户端）
    disconnect(m_privateClient, &OllamaClient::streamingChunk, this, &TestCaseFixerDialog::onAIChunk);
    disconnect(m_privateClient, &OllamaClient::streamingFinished, this, &TestCaseFixerDialog::onAIFinished);
    disconnect(m_privateClient, &OllamaClient::error, this, &TestCaseFixerDialog::onAIError);
    
    if (m_currentMode == Scanning) {
        applyAIScanResult();
    } else if (m_currentMode == Fixing) {
        applyAIFix();
    }
}

void TestCaseFixerDialog::applyAIScanResult()
{
    // 解析AI扫描结果
    QRegularExpression jsonRegex(R"(```json\s*(\{[\s\S]*?\})\s*```)");
    QRegularExpressionMatch match = jsonRegex.match(m_currentAIResponse);
    
    int idx = m_selectedIndices[m_currentScanIndex];
    QuestionItem &item = m_allQuestions[idx];
    
    if (!match.hasMatch()) {
        m_logView->append("⚠️ AI响应格式错误，跳过");
        // 更新列表项显示为未知
        QListWidgetItem *listItem = m_questionList->item(idx);
        listItem->setText(QString("❓ %1 (AI分析失败)").arg(item.title));
        listItem->setForeground(QColor("#868e96"));
        
        m_progressBar->setValue(m_currentScanIndex + 1);
        m_currentScanIndex++;
        scanNextQuestion();
        return;
    }
    
    QString jsonStr = match.captured(1);
    QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());
    
    if (!doc.isObject()) {
        m_logView->append("⚠️ JSON格式错误，跳过");
        m_progressBar->setValue(m_currentScanIndex + 1);
        m_currentScanIndex++;
        scanNextQuestion();
        return;
    }
    
    QJsonObject result = doc.object();
    bool hasIssues = result["hasIssues"].toBool();
    QJsonArray problematicArray = result["problematicIndices"].toArray();
    QString summary = result["summary"].toString();
    
    // 转换问题索引
    QVector<int> problematicIndices;
    for (const QJsonValue &val : problematicArray) {
        problematicIndices.append(val.toInt() - 1);  // 转换为0-based
    }
    
    item.hasIssues = hasIssues;
    item.problematicIndices = problematicIndices;
    
    // 更新列表项显示
    QListWidgetItem *listItem = m_questionList->item(idx);
    if (hasIssues) {
        m_questionsToFix.append(item);
        listItem->setText(QString("⚠️ %1 (%2个问题)").arg(item.title).arg(problematicIndices.size()));
        listItem->setForeground(QColor("#ff6b6b"));
        m_logView->append(QString("⚠️ 发现问题：%1").arg(summary));
    } else {
        listItem->setText(QString("✅ %1").arg(item.title));
        listItem->setForeground(QColor("#51cf66"));
        m_logView->append("✅ 正常");
    }
    
    m_progressBar->setValue(m_currentScanIndex + 1);
    m_currentScanIndex++;
    scanNextQuestion();
}

void TestCaseFixerDialog::applyAIFix()
{
    // 解析AI响应
    QRegularExpression jsonRegex(R"(```json\s*(\[[\s\S]*?\])\s*```)");
    QRegularExpressionMatch match = jsonRegex.match(m_currentAIResponse);
    
    if (!match.hasMatch()) {
        m_logView->append("❌ 错误：未找到有效的JSON格式");
        m_currentFixIndex++;
        fixNextQuestion();
        return;
    }
    
    QString jsonStr = match.captured(1);
    QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());
    
    if (!doc.isArray()) {
        m_logView->append("❌ 错误：JSON格式错误");
        m_currentFixIndex++;
        fixNextQuestion();
        return;
    }
    
    // 应用修复
    const QuestionItem &item = m_questionsToFix[m_currentFixIndex];
    QVector<TestCase> testCases = m_currentQuestion.testCases();
    QJsonArray array = doc.array();
    int fixedCount = 0;
    int validationErrors = 0;
    
    for (const QJsonValue &val : array) {
        if (!val.isObject()) continue;
        
        QJsonObject obj = val.toObject();
        int index = obj["index"].toInt() - 1;
        
        if (index < 0 || index >= testCases.size()) continue;
        
        QString newInput = obj["input"].toString();
        QString newOutput = obj["output"].toString();
        
        // 验证修复后的数据不包含文字描述或符号
        if (newInput.contains("...") || newInput.contains("（重复") || 
            newInput.contains("(重复") || newInput.contains("省略") ||
            newInput.contains("[...]") || newInput.contains("<省略>")) {
            m_logView->append(QString("⚠️ 警告：测试用例 %1 的输入仍包含文字描述，跳过").arg(index + 1));
            validationErrors++;
            continue;
        }
        
        if (newOutput.contains("...") || newOutput.contains("（重复") || 
            newOutput.contains("(重复") || newOutput.contains("省略")) {
            m_logView->append(QString("⚠️ 警告：测试用例 %1 的输出仍包含文字描述，跳过").arg(index + 1));
            validationErrors++;
            continue;
        }
        
        testCases[index].input = newInput;
        testCases[index].expectedOutput = newOutput;
        fixedCount++;
    }
    
    if (fixedCount == 0) {
        m_logView->append("❌ 错误：没有有效的修复数据");
        m_currentFixIndex++;
        fixNextQuestion();
        return;
    }
    
    m_currentQuestion.setTestCases(testCases);
    
    // 保存到文件
    if (saveFixedQuestion(m_currentQuestion, item.filePath)) {
        m_logView->append(QString("✅ 成功修复并保存 %1 个测试用例到文件").arg(fixedCount));
        if (validationErrors > 0) {
            m_logView->append(QString("⚠️ %1 个测试用例因包含文字描述被跳过").arg(validationErrors));
        }
        
        // 验证文件已保存
        QFile verifyFile(item.filePath);
        if (verifyFile.exists()) {
            m_logView->append(QString("✓ 已确认文件保存：%1").arg(item.filePath));
        }
        
        m_progressBar->setValue(m_currentFixIndex + 1);
        m_currentFixIndex++;
        fixNextQuestion();
    } else {
        m_logView->append("❌ 错误：保存文件失败");
        m_currentFixIndex++;
        fixNextQuestion();
    }
}

void TestCaseFixerDialog::onAIError(const QString &error)
{
    // 断开信号（使用私有客户端）
    disconnect(m_privateClient, &OllamaClient::streamingChunk, this, &TestCaseFixerDialog::onAIChunk);
    disconnect(m_privateClient, &OllamaClient::streamingFinished, this, &TestCaseFixerDialog::onAIFinished);
    disconnect(m_privateClient, &OllamaClient::error, this, &TestCaseFixerDialog::onAIError);
    
    m_logView->append(QString("❌ AI调用失败：%1").arg(error));
    
    if (m_currentMode == Scanning) {
        m_currentScanIndex++;
        scanNextQuestion();
    } else if (m_currentMode == Fixing) {
        m_currentFixIndex++;
        fixNextQuestion();
    }
}

void TestCaseFixerDialog::onStopFix()
{
    m_isScanning = false;
    m_isFixing = false;
    m_currentMode = Idle;
    m_statusLabel->setText("⏹ 已停止");
    m_statusLabel->setStyleSheet("color: orange; padding: 5px; font-size: 12px;");
    m_logView->append("\n⏹ 用户停止了操作\n");
    
    m_scanButton->setEnabled(true);
    m_fixButton->setEnabled(!m_questionsToFix.isEmpty());
    m_stopButton->setEnabled(false);
    m_progressBar->setVisible(false);
}

bool TestCaseFixerDialog::saveFixedQuestion(const Question &question, const QString &filePath)
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
