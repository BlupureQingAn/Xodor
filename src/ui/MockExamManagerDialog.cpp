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

MockExamManagerDialog::MockExamManagerDialog(const QVector<Question> &questions,
                                           OllamaClient *aiClient,
                                           QWidget *parent)
    : QDialog(parent)
    , m_questions(questions)
    , m_aiClient(aiClient)
{
    m_generator = new MockExamGenerator(aiClient, this);
    
    setupUI();
    setWindowTitle("模拟题库管理");
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
    
    loadExistingExams();
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
    
    // 题库分类选择
    m_categoryCombo = new QComboBox(this);
    m_categoryCombo->addItem("CCF", "ccf");
    m_categoryCombo->addItem("LeetCode", "leetcode");
    m_categoryCombo->addItem("自定义", "custom");
    
    // 分析按钮
    m_analyzeBtn = new QPushButton("分析题库", this);
    connect(m_analyzeBtn, &QPushButton::clicked, this, &MockExamManagerDialog::onAnalyzeBank);
    
    QHBoxLayout *categoryLayout = new QHBoxLayout();
    categoryLayout->addWidget(m_categoryCombo, 1);
    categoryLayout->addWidget(m_analyzeBtn);
    
    // 出题规律显示
    m_patternLabel = new QLabel("请先分析题库", this);
    m_patternLabel->setStyleSheet("color: #b0b0b0; padding: 10px; background: #1a1a1a; border-radius: 6px;");
    m_patternLabel->setWordWrap(true);
    
    // 生成数量
    m_examCountSpinBox = new QSpinBox(this);
    m_examCountSpinBox->setRange(1, 10);
    m_examCountSpinBox->setValue(2);
    m_examCountSpinBox->setSuffix(" 套");
    
    configLayout->addRow("题库分类:", categoryLayout);
    configLayout->addRow("出题规律:", m_patternLabel);
    configLayout->addRow("生成数量:", m_examCountSpinBox);

    // 生成按钮
    m_generateBtn = new QPushButton("开始生成", this);
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
    
    m_analyzeBtn->setStyleSheet(btnStyle);
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

void MockExamManagerDialog::onAnalyzeBank()
{
    m_currentCategory = m_categoryCombo->currentData().toString();
    
    m_logText->append(QString("🔍 开始分析 [%1] 题库...\n").arg(m_currentCategory));
    
    // 使用传入的题目列表
    if (m_questions.isEmpty()) {
        QMessageBox::warning(this, "提示", "当前题库为空，请先导入题目。");
        m_logText->append("❌ 题库为空\n");
        return;
    }
    
    // 分析题库
    m_currentPattern = m_generator->analyzeQuestionBank(m_questions, m_currentCategory);
    
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
    m_generateBtn->setEnabled(true);
    
    m_logText->append("✅ 题库分析完成\n");
    m_logText->append(patternText + "\n");
    
    // 保存规律
    QString bankPath = QString("基础题库/%1").arg(m_currentCategory);
    m_generator->savePattern(bankPath, m_currentPattern);
}

void MockExamManagerDialog::onGenerateExams()
{
    if (m_currentPattern.categoryName.isEmpty()) {
        QMessageBox::warning(this, "提示", "请先分析题库。");
        return;
    }
    
    if (!m_aiClient) {
        QMessageBox::warning(this, "提示", "AI服务未配置。");
        return;
    }
    
    int examCount = m_examCountSpinBox->value();
    
    m_generateBtn->setEnabled(false);
    m_analyzeBtn->setEnabled(false);
    m_progressBar->setVisible(true);
    m_progressBar->setValue(0);
    
    m_logText->append(QString("\n🚀 开始生成 %1 套模拟题...\n").arg(examCount));
    
    m_generator->generateMockExam(m_currentPattern, examCount);
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
    m_logText->append(QString("✅ 第 %1 套题生成完成，共 %2 道题\n")
        .arg(examIndex).arg(questions.size()));
    
    // 保存模拟题
    saveExam(questions, examIndex);
    
    // 刷新列表
    loadExistingExams();
}

void MockExamManagerDialog::onGenerationComplete(int totalExams)
{
    m_progressBar->setValue(100);
    m_logText->append(QString("\n🎉 所有模拟题生成完成！共 %1 套\n").arg(totalExams));
    
    m_generateBtn->setEnabled(true);
    m_analyzeBtn->setEnabled(true);
    
    QMessageBox::information(this, "生成完成",
        QString("成功生成 %1 套模拟题！").arg(totalExams));
}

void MockExamManagerDialog::onGenerationError(const QString &error)
{
    m_logText->append(QString("\n❌ 生成错误：%1\n").arg(error));
    m_progressBar->setValue(0);
    m_generateBtn->setEnabled(true);
    m_analyzeBtn->setEnabled(true);
    
    QMessageBox::critical(this, "生成失败", error);
}

void MockExamManagerDialog::loadExistingExams()
{
    m_examList->clear();
    
    QString mockPath = QString("人工模拟题库/%1").arg(m_currentCategory);
    QDir mockDir(mockPath);
    
    if (!mockDir.exists()) {
        return;
    }
    
    QStringList examDirs = mockDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    
    for (const QString &examDir : examDirs) {
        QString fullPath = mockDir.filePath(examDir);
        QDir dir(fullPath);
        
        int questionCount = dir.entryList(QStringList() << "*.md", QDir::Files).size();
        
        QListWidgetItem *item = new QListWidgetItem(
            QString("📝 %1 (%2 道题)").arg(examDir).arg(questionCount)
        );
        item->setData(Qt::UserRole, fullPath);
        m_examList->addItem(item);
    }
}

void MockExamManagerDialog::saveExam(const QVector<Question> &questions, int examIndex)
{
    QString examPath = getExamPath(m_currentCategory, examIndex);
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
            "# 模拟题 %1 - 答题说明\n\n"
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
        ).arg(examIndex)
         .arg(m_currentPattern.questionsPerExam)
         .arg(m_currentPattern.timeLimit)
         .arg(m_currentPattern.supportedLanguages.join(", "));
        
        readmeFile.write(readme.toUtf8());
        readmeFile.close();
    }
}

QString MockExamManagerDialog::getExamPath(const QString &category, int examIndex)
{
    return QString("人工模拟题库/%1/模拟题%2").arg(category).arg(examIndex);
}
