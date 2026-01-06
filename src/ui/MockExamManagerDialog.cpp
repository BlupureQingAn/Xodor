#include "MockExamManagerDialog.h"
#include "../core/QuestionBankManager.h"
#include "../ai/OllamaClient.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QFileDialog>
#include <QFile>
#include <QDir>

MockExamManagerDialog::MockExamManagerDialog(OllamaClient *aiClient,
                                           QWidget *parent)
    : QDialog(parent)
    , m_aiClient(aiClient)
{
    m_generator = new MockExamGenerator(aiClient, this);
    
    setupUI();
    setWindowTitle("AI模拟题库生成");
    resize(900, 700);
    
    // 连接信号
    connect(m_generator, &MockExamGenerator::progressUpdated,
            this, &MockExamManagerDialog::onProgressUpdated);
    connect(m_generator, &MockExamGenerator::examGenerated,
            this, &MockExamManagerDialog::onExamGenerated);
    connect(m_generator, &MockExamGenerator::generationComplete,
            this, &MockExamManagerDialog::onGenerationComplete);
    connect(m_generator, &MockExamGenerator::error,
            this, &MockExamManagerDialog::onGenerationError);
    
    // 加载可用的题库
    loadAvailableBanks();
}

void MockExamManagerDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(16);
    mainLayout->setContentsMargins(24, 24, 24, 24);
    
    // 标题
    QLabel *titleLabel = new QLabel("📚 模拟题库管理", this);
    titleLabel->setStyleSheet("font-size: 18pt; font-weight: bold; color: #e8e8e8;");
    
    // 生成配置区域
    QGroupBox *configGroup = new QGroupBox("生成配置", this);
    QFormLayout *configLayout = new QFormLayout(configGroup);
    
    // 题库选择
    m_bankCombo = new QComboBox(this);
    connect(m_bankCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MockExamManagerDialog::onBankSelectionChanged);
    
    // 题库信息显示
    m_bankInfoLabel = new QLabel("请选择一个题库", this);
    m_bankInfoLabel->setStyleSheet("color: #b0b0b0; padding: 10px; background: #1a1a1a; border-radius: 6px;");
    m_bankInfoLabel->setWordWrap(true);
    
    // 出题规律显示
    m_patternLabel = new QLabel("", this);
    m_patternLabel->setStyleSheet("color: #b0b0b0; padding: 10px; background: #1a1a1a; border-radius: 6px;");
    m_patternLabel->setWordWrap(true);
    m_patternLabel->setVisible(false);
    
    configLayout->addRow("选择样本题库:", m_bankCombo);
    configLayout->addRow("题库信息:", m_bankInfoLabel);
    configLayout->addRow("出题规律:", m_patternLabel);

    // 生成按钮
    m_generateBtn = new QPushButton("生成模拟题库", this);
    m_generateBtn->setEnabled(false);
    connect(m_generateBtn, &QPushButton::clicked, this, &MockExamManagerDialog::onGenerateExams);
    
    configLayout->addRow("", m_generateBtn);
    
    // 已有模拟题列表
    QGroupBox *examGroup = new QGroupBox("已有模拟题", this);
    QVBoxLayout *examLayout = new QVBoxLayout(examGroup);
    
    m_examList = new QListWidget(this);
    m_examList->setStyleSheet(R"(
        QListWidget {
            background-color: #1a1a1a;
            color: #e8e8e8;
            border: 2px solid #3a3a3a;
            border-radius: 8px;
            padding: 8px;
        }
        QListWidget::item {
            padding: 8px;
            border-radius: 4px;
        }
        QListWidget::item:selected {
            background-color: #660000;
            outline: none;
        }
        QListWidget::item:selected:hover {
            background-color: #880000;
        }
        QListWidget::item:hover {
            background-color: #3a3a3a;
        }
    )");
    
    QHBoxLayout *examBtnLayout = new QHBoxLayout();
    m_viewBtn = new QPushButton("查看", this);
    m_deleteBtn = new QPushButton("删除", this);
    m_exportBtn = new QPushButton("导出", this);
    
    connect(m_viewBtn, &QPushButton::clicked, this, &MockExamManagerDialog::onViewExam);
    connect(m_deleteBtn, &QPushButton::clicked, this, &MockExamManagerDialog::onDeleteExam);
    connect(m_exportBtn, &QPushButton::clicked, this, &MockExamManagerDialog::onExportExam);
    
    examBtnLayout->addWidget(m_viewBtn);
    examBtnLayout->addWidget(m_deleteBtn);
    examBtnLayout->addWidget(m_exportBtn);
    examBtnLayout->addStretch();
    
    examLayout->addWidget(m_examList);
    examLayout->addLayout(examBtnLayout);
    
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
    QLabel *logLabel = new QLabel("📋 操作日志:", this);
    m_logText = new QTextEdit(this);
    m_logText->setReadOnly(true);
    m_logText->setMaximumHeight(150);
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
    
    // 关闭按钮
    QPushButton *closeBtn = new QPushButton("关闭", this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    
    QHBoxLayout *closeBtnLayout = new QHBoxLayout();
    closeBtnLayout->addStretch();
    closeBtnLayout->addWidget(closeBtn);
    
    // 布局
    mainLayout->addWidget(titleLabel);
    mainLayout->addWidget(configGroup);
    mainLayout->addWidget(examGroup, 1);
    mainLayout->addWidget(m_progressBar);
    mainLayout->addWidget(logLabel);
    mainLayout->addWidget(m_logText);
    mainLayout->addLayout(closeBtnLayout);
    
    // 样式
    QString btnStyle = R"(
        QPushButton {
            background-color: #660000;
            color: white;
            border: none;
            border-radius: 8px;
            padding: 10px 24px;
            font-weight: 600;
            font-size: 10pt;
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
    
    m_generateBtn->setStyleSheet(btnStyle);
    m_viewBtn->setStyleSheet(btnStyle);
    m_deleteBtn->setStyleSheet(btnStyle);
    m_exportBtn->setStyleSheet(btnStyle);
    closeBtn->setStyleSheet(btnStyle);
    
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
    )");
}

void MockExamManagerDialog::loadAvailableBanks()
{
    m_bankCombo->clear();
    m_bankCombo->addItem("-- 请选择题库 --", "");
    
    // 从QuestionBankManager获取所有已注册的题库
    QDir baseDir("data/基础题库");
    if (!baseDir.exists()) {
        return;
    }
    
    QStringList banks = baseDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    
    for (const QString &bankName : banks) {
        // 过滤掉ai模拟题库文件夹
        if (bankName == "ai模拟题库") {
            continue;
        }
        
        // 统计题目数量
        QString bankPath = baseDir.filePath(bankName);
        QDir bankDir(bankPath);
        int questionCount = bankDir.entryList(QStringList() << "*.md", QDir::Files | QDir::NoDotAndDotDot).count();
        
        m_bankCombo->addItem(QString("%1 (%2 道题)").arg(bankName).arg(questionCount), bankName);
    }
}

void MockExamManagerDialog::loadBankQuestions(const QString &bankName)
{
    m_currentQuestions.clear();
    
    QString bankPath = QString("data/基础题库/%1").arg(bankName);
    QDir bankDir(bankPath);
    
    if (!bankDir.exists()) {
        return;
    }
    
    // 加载所有题目文件
    QFileInfoList files = bankDir.entryInfoList(QStringList() << "*.md", QDir::Files);
    
    for (const QFileInfo &fileInfo : files) {
        Question q = Question::fromMarkdownFile(fileInfo.absoluteFilePath());
        if (!q.id().isEmpty()) {
            m_currentQuestions.append(q);
        }
    }
}

void MockExamManagerDialog::onBankSelectionChanged(int index)
{
    QString bankName = m_bankCombo->currentData().toString();
    
    if (bankName.isEmpty()) {
        m_bankInfoLabel->setText("请选择一个题库");
        m_patternLabel->setVisible(false);
        m_generateBtn->setEnabled(false);
        m_currentBankName.clear();
        m_currentQuestions.clear();
        loadExistingExams();
        return;
    }
    
    m_currentBankName = bankName;
    m_logText->append(QString("📚 选择题库：%1\n").arg(bankName));
    
    // 加载题目
    loadBankQuestions(bankName);
    
    if (m_currentQuestions.isEmpty()) {
        m_bankInfoLabel->setText("⚠️ 该题库为空，无法生成模拟题");
        m_patternLabel->setVisible(false);
        m_generateBtn->setEnabled(false);
        m_logText->append("❌ 题库为空\n");
        return;
    }
    
    // 检查是否有导入规则文件
    bool hasRules = m_generator->hasSourceBankRules(bankName);
    
    // 分析题库
    m_currentPattern = m_generator->analyzeQuestionBank(m_currentQuestions, bankName);
    
    // 显示题库信息
    QString infoText = QString(
        "✅ 题库已加载\n"
        "• 题目总数：%1 道\n"
        "• 导入规则：%2"
    ).arg(m_currentQuestions.size())
     .arg(hasRules ? "✅ 已找到" : "⚠️ 未找到");
    
    m_bankInfoLabel->setText(infoText);
    
    // 显示分析结果
    QString patternText = QString(
        "📊 分析结果：\n"
        "• 每套题目数：%1 道\n"
        "• 时间限制：%2 分钟\n"
        "• 难度分布：简单 %3%，中等 %4%，困难 %5%\n"
        "• 主要知识点：%6"
    ).arg(m_currentPattern.questionsPerExam)
     .arg(m_currentPattern.timeLimit)
     .arg(m_currentPattern.difficultyRatio[Difficulty::Easy] * 100, 0, 'f', 0)
     .arg(m_currentPattern.difficultyRatio[Difficulty::Medium] * 100, 0, 'f', 0)
     .arg(m_currentPattern.difficultyRatio[Difficulty::Hard] * 100, 0, 'f', 0)
     .arg(m_currentPattern.topicRatio.keys().mid(0, 5).join(", "));
    
    m_patternLabel->setText(patternText);
    m_patternLabel->setVisible(true);
    
    m_generateBtn->setEnabled(true);
    
    m_logText->append("✅ 题库分析完成\n");
    
    if (!hasRules) {
        m_logText->append(QString("⚠️ 未找到导入规则文件：data/config/%1_parse_rule.json\n").arg(bankName));
        m_logText->append("💡 提示：将使用基础分析结果生成模拟题\n");
    } else {
        m_logText->append(QString("✅ 找到导入规则文件：data/config/%1_parse_rule.json\n").arg(bankName));
    }
    
    // 加载已有的模拟题
    loadExistingExams();
}

void MockExamManagerDialog::onGenerateExams()
{
    if (m_currentBankName.isEmpty()) {
        QMessageBox::warning(this, "提示", "请先选择一个题库。");
        return;
    }
    
    if (!m_aiClient) {
        QMessageBox::warning(this, "提示", "AI服务未配置。");
        return;
    }
    
    // 检查是否已有模拟题
    QString mockPath = QString("data/基础题库/ai模拟题库/%1-模拟").arg(m_currentBankName);
    if (QDir(mockPath).exists()) {
        int ret = QMessageBox::question(this, "确认覆盖",
            QString("题库 '%1' 已有模拟题库。\n\n"
                   "生成新的模拟题将覆盖原有内容。\n\n"
                   "是否继续？").arg(m_currentBankName),
            QMessageBox::Yes | QMessageBox::No);
        
        if (ret == QMessageBox::No) {
            m_logText->append("❌ 用户取消操作\n");
            return;
        }
    }
    
    m_generateBtn->setEnabled(false);
    m_bankCombo->setEnabled(false);
    m_progressBar->setVisible(true);
    m_progressBar->setValue(0);
    
    m_logText->append(QString("\n🚀 开始为 [%1] 生成模拟题库...\n").arg(m_currentBankName));
    
    // 生成1套模拟题（包含多道题目）
    m_generator->generateMockExam(m_currentPattern, 1);
}

void MockExamManagerDialog::onViewExam()
{
    QListWidgetItem *item = m_examList->currentItem();
    if (!item) {
        QMessageBox::information(this, "提示", "请先选择一套模拟题。");
        return;
    }
    
    QString examPath = item->data(Qt::UserRole).toString();
    m_logText->append(QString("👀 查看模拟题：%1\n").arg(item->text()));
    
    // TODO: 打开模拟题详情对话框
    QMessageBox::information(this, "查看模拟题", 
        QString("模拟题路径：%1\n\n功能开发中...").arg(examPath));
}

void MockExamManagerDialog::onDeleteExam()
{
    QListWidgetItem *item = m_examList->currentItem();
    if (!item) {
        QMessageBox::information(this, "提示", "请先选择一套模拟题。");
        return;
    }
    
    int ret = QMessageBox::question(this, "确认删除",
        QString("确定要删除 \"%1\" 吗？").arg(item->text()));
    
    if (ret == QMessageBox::Yes) {
        QString examPath = item->data(Qt::UserRole).toString();
        QDir dir(examPath);
        if (dir.removeRecursively()) {
            delete item;
            m_logText->append(QString("🗑️ 已删除：%1\n").arg(examPath));
        } else {
            QMessageBox::warning(this, "删除失败", "无法删除模拟题文件夹。");
        }
    }
}

void MockExamManagerDialog::onExportExam()
{
    QListWidgetItem *item = m_examList->currentItem();
    if (!item) {
        QMessageBox::information(this, "提示", "请先选择一套模拟题。");
        return;
    }
    
    QString exportPath = QFileDialog::getExistingDirectory(this, "选择导出目录");
    if (exportPath.isEmpty()) {
        return;
    }
    
    m_logText->append(QString("📤 导出模拟题到：%1\n").arg(exportPath));
    
    // TODO: 实现导出功能
    QMessageBox::information(this, "导出", "导出功能开发中...");
}

void MockExamManagerDialog::onProgressUpdated(int percentage, const QString &message)
{
    m_progressBar->setValue(percentage);
    m_logText->append(QString("⏳ %1\n").arg(message));
}

void MockExamManagerDialog::onExamGenerated(const QVector<Question> &questions, int examIndex)
{
    m_logText->append(QString("✅ 模拟题生成完成，共 %1 道题\n")
        .arg(questions.size()));
    
    // 保存模拟题
    saveExam(questions);
    
    // 刷新列表
    loadExistingExams();
}

void MockExamManagerDialog::onGenerationComplete(int totalExams)
{
    m_progressBar->setValue(100);
    m_logText->append(QString("\n🎉 模拟题库生成完成！\n"));
    
    m_generateBtn->setEnabled(true);
    m_bankCombo->setEnabled(true);
    
    // 刷新已有模拟题列表
    loadExistingExams();
    
    QMessageBox::information(this, "生成完成",
        QString("成功为 '%1' 生成模拟题库！\n\n"
               "保存位置：data/基础题库/ai模拟题库/%1-模拟/").arg(m_currentBankName));
}

void MockExamManagerDialog::onGenerationError(const QString &error)
{
    m_logText->append(QString("\n❌ 生成错误：%1\n").arg(error));
    m_progressBar->setValue(0);
    m_generateBtn->setEnabled(true);
    m_bankCombo->setEnabled(true);
    
    QMessageBox::critical(this, "生成失败", error);
}

void MockExamManagerDialog::loadExistingExams()
{
    m_examList->clear();
    
    if (m_currentBankName.isEmpty()) {
        return;
    }
    
    // 从 ai模拟题库 目录加载当前题库的模拟题
    QString mockPath = QString("data/基础题库/ai模拟题库/%1-模拟").arg(m_currentBankName);
    QDir mockDir(mockPath);
    
    if (!mockDir.exists()) {
        return;
    }
    
    int questionCount = mockDir.entryList(QStringList() << "*.md", QDir::Files).size();
    
    if (questionCount > 0) {
        QListWidgetItem *item = new QListWidgetItem(
            QString("📝 %1-模拟 (%2 道题)").arg(m_currentBankName).arg(questionCount)
        );
        item->setData(Qt::UserRole, mockPath);
        m_examList->addItem(item);
    }
}

void MockExamManagerDialog::saveExam(const QVector<Question> &questions)
{
    QString examPath = getExamPath();
    QDir dir;
    dir.mkpath(examPath);
    
    // 保存每道题
    for (int i = 0; i < questions.size(); ++i) {
        const Question &q = questions[i];
        QString fileName = QString("第%1题.md").arg(i + 1);
        QString filePath = QDir(examPath).filePath(fileName);
        
        QFile file(filePath);
        if (file.open(QIODevice::WriteOnly)) {
            QString content = QString("# %1\n\n%2\n\n## 测试用例\n\n")
                .arg(q.title())
                .arg(q.description());
            
            for (int j = 0; j < q.testCases().size(); ++j) {
                const TestCase &tc = q.testCases()[j];
                content += QString("### 测试 %1：%2\n\n输入：\n```\n%3\n```\n\n输出：\n```\n%4\n```\n\n")
                    .arg(j + 1)
                    .arg(tc.description)
                    .arg(tc.input)
                    .arg(tc.expectedOutput);
            }
            
            file.write(content.toUtf8());
            file.close();
        }
    }
    
    // 保存答题说明
    QString readmePath = QDir(examPath).filePath("答题说明.md");
    QFile readmeFile(readmePath);
    if (readmeFile.open(QIODevice::WriteOnly)) {
        QString readme = QString(
            "# %1-模拟 - 答题说明\n\n"
            "## 考试信息\n\n"
            "- 题目数量：%2 道\n"
            "- 时间限制：%3 分钟\n"
            "- 支持语言：%4\n\n"
            "## 答题规则\n\n"
            "1. 按题号顺序答题，不可跳题\n"
            "2. 每道题有时间限制和内存限制\n"
            "3. 通过所有测试用例即为通过\n"
            "4. 可以使用 AI 辅助功能获取提示\n\n"
            "## 测试说明\n\n"
            "- 点击\"测试\"按钮运行所有测试用例\n"
            "- 测试结果会显示通过/失败状态\n"
            "- 失败的测试会显示详细错误信息\n\n"
            "祝你答题顺利！🎯\n"
        ).arg(m_currentBankName)
         .arg(m_currentPattern.questionsPerExam)
         .arg(m_currentPattern.timeLimit)
         .arg(m_currentPattern.supportedLanguages.join(", "));
        
        readmeFile.write(readme.toUtf8());
        readmeFile.close();
    }
}

QString MockExamManagerDialog::getExamPath()
{
    // 模拟题保存到：data/基础题库/ai模拟题库/{bankName}-模拟/
    QString mockBankName = QString("%1-模拟").arg(m_currentBankName);
    return QString("data/基础题库/ai模拟题库/%1").arg(mockBankName);
}
