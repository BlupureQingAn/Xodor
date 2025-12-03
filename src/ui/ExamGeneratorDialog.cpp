#include "ExamGeneratorDialog.h"
#include "../ai/OllamaClient.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

ExamGeneratorDialog::ExamGeneratorDialog(const QVector<Question> &existingQuestions,
                                       OllamaClient *aiClient, QWidget *parent)
    : QDialog(parent)
    , m_aiClient(aiClient)
    , m_existingQuestions(existingQuestions)
    , m_success(false)
{
    setupUI();
    setWindowTitle("生成模拟题");
    resize(700, 600);
    
    // 连接AI信号
    if (m_aiClient) {
        connect(m_aiClient, &OllamaClient::codeAnalysisReady,
                this, &ExamGeneratorDialog::onAIResponse, Qt::UniqueConnection);
        connect(m_aiClient, &OllamaClient::error,
                this, &ExamGeneratorDialog::onAIError, Qt::UniqueConnection);
    }
}

void ExamGeneratorDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(16);
    mainLayout->setContentsMargins(24, 24, 24, 24);
    
    // 标题
    QLabel *titleLabel = new QLabel("🎯 AI生成模拟题", this);
    titleLabel->setStyleSheet("font-size: 18pt; font-weight: bold; color: #e8e8e8;");
    
    // 配置区域
    QGroupBox *configGroup = new QGroupBox("生成配置", this);
    QFormLayout *configLayout = new QFormLayout(configGroup);
    configLayout->setSpacing(12);
    
    // 题目数量
    m_countSpinBox = new QSpinBox(this);
    m_countSpinBox->setRange(1, 20);
    m_countSpinBox->setValue(5);
    m_countSpinBox->setSuffix(" 道题");
    
    // 难度选择
    m_difficultyCombo = new QComboBox(this);
    m_difficultyCombo->addItem("混合难度", "mixed");
    m_difficultyCombo->addItem("简单", "easy");
    m_difficultyCombo->addItem("中等", "medium");
    m_difficultyCombo->addItem("困难", "hard");
    
    // 包含测试用例
    m_includeTestsCheckBox = new QCheckBox("自动生成测试用例", this);
    m_includeTestsCheckBox->setChecked(true);
    
    configLayout->addRow("题目数量:", m_countSpinBox);
    configLayout->addRow("难度:", m_difficultyCombo);
    configLayout->addRow("", m_includeTestsCheckBox);
    
    // 说明文本
    QLabel *infoLabel = new QLabel(
        "💡 提示：\n"
        "• AI将基于现有题库生成类似风格的新题目\n"
        "• 生成的题目会包含完整描述和测试用例\n"
        "• 建议题目数量不超过10道，以保证质量\n"
        "• 生成时间约1-2分钟",
        this
    );
    infoLabel->setStyleSheet("color: #b0b0b0; font-size: 9pt; padding: 10px;");
    infoLabel->setWordWrap(true);
    
    // 状态标签
    m_statusLabel = new QLabel("准备生成", this);
    m_statusLabel->setStyleSheet("color: #b0b0b0; font-size: 10pt;");
    
    // 进度条
    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setVisible(false);
    m_progressBar->setStyleSheet(R"(
        QProgressBar {
            border: 2px solid #3a3a3a;
            border-radius: 8px;
            background-color: #1e1e1e;
            text-align: center;
            color: #e8e8e8;
            height: 28px;
        }
        QProgressBar::chunk {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 #660000, stop:1 #aa0000);
            border-radius: 6px;
        }
    )");
    
    // 日志区域
    QLabel *logLabel = new QLabel("📋 生成日志:", this);
    logLabel->setStyleSheet("color: #e8e8e8; font-weight: bold;");
    
    m_logText = new QTextEdit(this);
    m_logText->setReadOnly(true);
    m_logText->setStyleSheet(R"(
        QTextEdit {
            background-color: #1a1a1a;
            color: #e8e8e8;
            border: 2px solid #3a3a3a;
            border-radius: 8px;
            padding: 10px;
            font-family: 'Consolas', 'Monaco', monospace;
            font-size: 9pt;
        }
    )");
    
    // 按钮
    QHBoxLayout *btnLayout = new QHBoxLayout();
    m_generateBtn = new QPushButton("开始生成", this);
    m_closeBtn = new QPushButton("关闭", this);
    
    QString btnStyle = R"(
        QPushButton {
            background-color: #660000;
            color: white;
            border: none;
            border-radius: 10px;
            padding: 12px 28px;
            font-weight: 600;
            font-size: 10pt;
            min-width: 100px;
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
    
    m_generateBtn->setStyleSheet(btnStyle);
    m_closeBtn->setStyleSheet(btnStyle);
    
    connect(m_generateBtn, &QPushButton::clicked, this, &ExamGeneratorDialog::onGenerateClicked);
    connect(m_closeBtn, &QPushButton::clicked, this, &QDialog::reject);
    
    btnLayout->addStretch();
    btnLayout->addWidget(m_generateBtn);
    btnLayout->addWidget(m_closeBtn);
    
    // 布局
    mainLayout->addWidget(titleLabel);
    mainLayout->addWidget(configGroup);
    mainLayout->addWidget(infoLabel);
    mainLayout->addWidget(m_statusLabel);
    mainLayout->addWidget(m_progressBar);
    mainLayout->addWidget(logLabel);
    mainLayout->addWidget(m_logText, 1);
    mainLayout->addLayout(btnLayout);
    
    // 对话框样式
    setStyleSheet(R"(
        QDialog {
            background-color: #242424;
        }
        QLabel {
            color: #e8e8e8;
        }
        QGroupBox {
            color: #e8e8e8;
            border: 2px solid #4a4a4a;
            border-radius: 10px;
            margin-top: 12px;
            padding-top: 12px;
            font-weight: bold;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 12px;
            padding: 0 8px;
        }
        QSpinBox, QComboBox {
            background-color: #1e1e1e;
            color: #e8e8e8;
            border: 2px solid #3a3a3a;
            border-radius: 6px;
            padding: 6px;
            min-height: 24px;
        }
        QSpinBox:focus, QComboBox:focus {
            border-color: #660000;
        }
        QCheckBox {
            color: #e8e8e8;
        }
        QCheckBox::indicator {
            width: 18px;
            height: 18px;
            border: 2px solid #3a3a3a;
            border-radius: 4px;
            background-color: #1e1e1e;
        }
        QCheckBox::indicator:checked {
            background-color: #660000;
            border-color: #660000;
        }
    )");
}

void ExamGeneratorDialog::onGenerateClicked()
{
    if (m_existingQuestions.isEmpty()) {
        QMessageBox::warning(this, "提示", "当前没有题库，无法生成模拟题。\n\n请先导入题库。");
        return;
    }
    
    if (!m_aiClient) {
        QMessageBox::warning(this, "提示", "AI服务未配置。\n\n请在设置中配置Ollama服务。");
        return;
    }
    
    // 禁用生成按钮
    m_generateBtn->setEnabled(false);
    m_progressBar->setVisible(true);
    m_progressBar->setValue(30);
    
    m_statusLabel->setText("🤖 正在生成模拟题...");
    m_logText->append("🚀 开始生成模拟题\n");
    m_logText->append(QString("📊 配置: %1道题, 难度: %2\n")
        .arg(m_countSpinBox->value())
        .arg(m_difficultyCombo->currentText()));
    
    // 构建提示词
    QString prompt = buildPrompt();
    
    m_logText->append("⏳ 发送AI请求...\n");
    m_logText->append(QString("📝 提示词长度: %1 字符\n\n").arg(prompt.length()));
    
    // 发送给AI
    m_aiClient->analyzeCode("", prompt);
    
    m_progressBar->setValue(50);
}

QString ExamGeneratorDialog::buildPrompt()
{
    int count = m_countSpinBox->value();
    QString difficulty = m_difficultyCombo->currentData().toString();
    bool includeTests = m_includeTestsCheckBox->isChecked();
    
    // 分析现有题库
    QStringList topics;
    QMap<QString, int> tagCount;
    
    for (const Question &q : m_existingQuestions) {
        for (const QString &tag : q.tags()) {
            tagCount[tag]++;
        }
    }
    
    // 获取最常见的标签
    QList<QString> sortedTags = tagCount.keys();
    std::sort(sortedTags.begin(), sortedTags.end(), [&tagCount](const QString &a, const QString &b) {
        return tagCount[a] > tagCount[b];
    });
    
    if (sortedTags.size() > 5) {
        sortedTags = sortedTags.mid(0, 5);
    }
    
    QString prompt = R"(
你是一个专业的编程题目生成助手。请基于以下题库风格，生成一套全新的模拟题。

现有题库分析：
- 题目总数: %1 道
- 主要标签: %2
- 难度分布: 简单/中等/困难

生成要求：
1. 生成 %3 道全新的编程题目
2. 难度要求: %4
3. 题目要求：
   - 题目描述清晰完整
   - 包含输入输出说明
   - 包含约束条件
   - 风格与现有题库相似
4. 测试用例要求：
   - 每道题至少5个测试用例
   - 包含基本测试、边界条件、特殊情况
   - 每个用例包含描述

JSON格式：
{
  "questions": [
    {
      "title": "题目标题",
      "difficulty": "简单/中等/困难",
      "description": "完整的题目描述，包括：\n- 问题描述\n- 输入格式\n- 输出格式\n- 约束条件\n- 示例说明",
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
          "description": "边界条件"
        },
        {
          "input": "输入数据",
          "output": "期望输出",
          "description": "特殊情况"
        }
      ]
    }
  ]
}

注意事项：
- 题目要有创新性，不要直接复制现有题目
- 题目难度要合理，符合要求
- 测试用例要全面，覆盖各种情况
- 返回纯JSON，不要其他文字

请开始生成。
)";
    
    QString difficultyText;
    if (difficulty == "mixed") {
        difficultyText = "混合难度（简单、中等、困难各占一定比例）";
    } else if (difficulty == "easy") {
        difficultyText = "全部为简单题";
    } else if (difficulty == "medium") {
        difficultyText = "全部为中等题";
    } else {
        difficultyText = "全部为困难题";
    }
    
    prompt = prompt
        .arg(m_existingQuestions.size())
        .arg(sortedTags.join(", "))
        .arg(count)
        .arg(difficultyText);
    
    return prompt;
}

void ExamGeneratorDialog::onAIResponse(const QString &response)
{
    m_logText->append("✅ AI响应接收完成\n");
    m_progressBar->setValue(70);
    
    m_statusLabel->setText("🔍 正在解析结果...");
    m_logText->append("📝 解析AI返回的题目...\n\n");
    
    parseAIResponse(response);
}

void ExamGeneratorDialog::parseAIResponse(const QString &response)
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
        m_logText->append("❌ JSON解析失败\n");
        m_statusLabel->setText("生成失败");
        m_progressBar->setValue(0);
        m_generateBtn->setEnabled(true);
        
        QMessageBox::critical(this, "解析失败", 
            "无法解析AI返回的结果。\n\n请检查AI服务状态或重试。");
        return;
    }
    
    QJsonObject root = doc.object();
    QJsonArray questionsArray = root["questions"].toArray();
    
    m_logText->append(QString("✅ 成功解析 %1 道题目\n\n").arg(questionsArray.size()));
    
    for (const QJsonValue &val : questionsArray) {
        QJsonObject qObj = val.toObject();
        
        Question q;
        q.setId(QString("exam_%1").arg(qHash(qObj["title"].toString())));
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
        q.setTestCases(testCases);
        q.setType(QuestionType::Code);
        
        m_generatedQuestions.append(q);
        
        m_logText->append(QString("  ✓ %1 [%2] - %3个测试用例\n")
            .arg(q.title())
            .arg(diffStr)
            .arg(testCases.size()));
    }
    
    m_progressBar->setValue(100);
    m_statusLabel->setText(QString("✅ 成功生成 %1 道题目").arg(m_generatedQuestions.size()));
    m_logText->append(QString("\n🎉 模拟题生成完成！共 %1 道题目\n").arg(m_generatedQuestions.size()));
    
    m_success = true;
    m_generateBtn->setEnabled(false);
    m_closeBtn->setText("完成");
    
    QMessageBox::information(this, "生成成功",
        QString("成功生成 %1 道模拟题！\n\n"
                "点击\"完成\"将题目添加到题库。").arg(m_generatedQuestions.size()));
}

void ExamGeneratorDialog::onAIError(const QString &error)
{
    m_logText->append(QString("\n❌ AI错误: %1\n").arg(error));
    m_statusLabel->setText("生成失败");
    m_progressBar->setValue(0);
    m_generateBtn->setEnabled(true);
    
    QMessageBox::critical(this, "生成失败",
        QString("AI生成过程中出现错误：\n\n%1\n\n"
                "请检查AI服务状态或重试。").arg(error));
}
