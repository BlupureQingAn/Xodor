#include "QuestionEditorDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QMessageBox>
#include <QFileDialog>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QCryptographicHash>
#include <QScrollArea>
#include <QRegularExpression>

// ============ TestCaseItem 实现 ============

TestCaseItem::TestCaseItem(int index, QWidget *parent)
    : QWidget(parent)
    , m_index(index)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(6);
    
    // 标题栏
    QHBoxLayout *headerLayout = new QHBoxLayout();
    QLabel *titleLabel = new QLabel(QString("测试用例 #%1").arg(index + 1), this);
    titleLabel->setStyleSheet("font-weight: bold; color: #e8e8e8;");
    
    m_removeButton = new QPushButton("✖ 删除", this);
    m_removeButton->setStyleSheet(R"(
        QPushButton {
            background-color: #8b0000;
            color: white;
            border: none;
            border-radius: 4px;
            padding: 4px 12px;
        }
        QPushButton:hover {
            background-color: #a00000;
        }
    )");
    
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    headerLayout->addWidget(m_removeButton);
    
    // 输入输出编辑器
    QHBoxLayout *ioLayout = new QHBoxLayout();
    
    QVBoxLayout *inputLayout = new QVBoxLayout();
    QLabel *inputLabel = new QLabel("输入:", this);
    inputLabel->setStyleSheet("color: #e8e8e8;");
    m_inputEdit = new QTextEdit(this);
    m_inputEdit->setPlaceholderText("输入测试数据...");
    m_inputEdit->setMaximumHeight(100);
    m_inputEdit->setStyleSheet(R"(
        QTextEdit {
            background-color: #2d2d2d;
            color: #e8e8e8;
            border: 1px solid #3a3a3a;
            border-radius: 4px;
            padding: 4px;
            font-family: 'Consolas', 'Courier New', monospace;
        }
    )");
    inputLayout->addWidget(inputLabel);
    inputLayout->addWidget(m_inputEdit);
    
    QVBoxLayout *outputLayout = new QVBoxLayout();
    QLabel *outputLabel = new QLabel("期望输出:", this);
    outputLabel->setStyleSheet("color: #e8e8e8;");
    m_outputEdit = new QTextEdit(this);
    m_outputEdit->setPlaceholderText("期望的输出结果...");
    m_outputEdit->setMaximumHeight(100);
    m_outputEdit->setStyleSheet(R"(
        QTextEdit {
            background-color: #2d2d2d;
            color: #e8e8e8;
            border: 1px solid #3a3a3a;
            border-radius: 4px;
            padding: 4px;
            font-family: 'Consolas', 'Courier New', monospace;
        }
    )");
    outputLayout->addWidget(outputLabel);
    outputLayout->addWidget(m_outputEdit);
    
    ioLayout->addLayout(inputLayout);
    ioLayout->addLayout(outputLayout);
    
    mainLayout->addLayout(headerLayout);
    mainLayout->addLayout(ioLayout);
    
    // 样式
    setStyleSheet(R"(
        TestCaseItem {
            background-color: #323232;
            border: 1px solid #3a3a3a;
            border-radius: 6px;
        }
    )");
    
    connect(m_removeButton, &QPushButton::clicked, this, &TestCaseItem::removeRequested);
}

void TestCaseItem::setTestCase(const TestCase &testCase)
{
    m_inputEdit->setPlainText(testCase.input);
    m_outputEdit->setPlainText(testCase.expectedOutput);
}

TestCase TestCaseItem::getTestCase() const
{
    TestCase testCase;
    testCase.input = m_inputEdit->toPlainText();
    testCase.expectedOutput = m_outputEdit->toPlainText();
    return testCase;
}

// ============ QuestionEditorDialog 实现 ============

QuestionEditorDialog::QuestionEditorDialog(Mode mode, QWidget *parent)
    : QDialog(parent)
    , m_mode(mode)
{
    setupUI();
    setupConnections();
    
    QString title;
    switch (mode) {
        case CreateMode:
            title = "新建题目";
            break;
        case EditMode:
            title = "编辑题目";
            break;
        case ImportMode:
            title = "导入题目";
            break;
    }
    setWindowTitle(title);
    
    resize(900, 700);
}

QuestionEditorDialog::QuestionEditorDialog(const Question &question, QWidget *parent)
    : QDialog(parent)
    , m_mode(EditMode)
{
    setupUI();
    setupConnections();
    setQuestion(question);
    setWindowTitle("编辑题目");
    resize(900, 700);
}

void QuestionEditorDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);
    
    // 创建滚动区域
    QScrollArea *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet("QScrollArea { border: none; background-color: #242424; }");
    
    QWidget *contentWidget = new QWidget();
    QVBoxLayout *contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setSpacing(12);
    
    // 基本信息组
    QGroupBox *basicGroup = new QGroupBox("基本信息", this);
    basicGroup->setStyleSheet(R"(
        QGroupBox {
            color: #e8e8e8;
            border: 1px solid #3a3a3a;
            border-radius: 6px;
            margin-top: 12px;
            padding-top: 12px;
            font-weight: bold;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 10px;
            padding: 0 5px;
        }
    )");
    
    QFormLayout *basicLayout = new QFormLayout(basicGroup);
    basicLayout->setSpacing(10);
    
    m_titleEdit = new QLineEdit(this);
    m_titleEdit->setPlaceholderText("输入题目标题...");
    m_titleEdit->setStyleSheet(R"(
        QLineEdit {
            background-color: #2d2d2d;
            color: #e8e8e8;
            border: 1px solid #3a3a3a;
            border-radius: 4px;
            padding: 8px;
        }
        QLineEdit:focus {
            border: 1px solid #660000;
        }
    )");
    
    m_difficultyCombo = new QComboBox(this);
    m_difficultyCombo->addItems({"简单", "中等", "困难"});
    m_difficultyCombo->setStyleSheet(R"(
        QComboBox {
            background-color: #2d2d2d;
            color: #e8e8e8;
            border: 1px solid #3a3a3a;
            border-radius: 4px;
            padding: 8px;
        }
        QComboBox:focus {
            border: 1px solid #660000;
        }
        QComboBox::drop-down {
            border: none;
        }
        QComboBox QAbstractItemView {
            background-color: #2d2d2d;
            color: #e8e8e8;
            selection-background-color: #660000;
        }
    )");
    
    m_tagsEdit = new QLineEdit(this);
    m_tagsEdit->setPlaceholderText("标签（用逗号分隔，如：数组,动态规划）");
    m_tagsEdit->setStyleSheet(m_titleEdit->styleSheet());
    
    basicLayout->addRow("题目标题:", m_titleEdit);
    basicLayout->addRow("难度:", m_difficultyCombo);
    basicLayout->addRow("标签:", m_tagsEdit);
    
    // 题目描述组
    QGroupBox *descGroup = new QGroupBox("题目描述", this);
    descGroup->setStyleSheet(basicGroup->styleSheet());
    
    QVBoxLayout *descLayout = new QVBoxLayout(descGroup);
    m_descriptionEdit = new QTextEdit(this);
    m_descriptionEdit->setPlaceholderText("输入题目描述...\n\n支持 Markdown 格式");
    m_descriptionEdit->setMinimumHeight(150);
    m_descriptionEdit->setStyleSheet(R"(
        QTextEdit {
            background-color: #2d2d2d;
            color: #e8e8e8;
            border: 1px solid #3a3a3a;
            border-radius: 4px;
            padding: 8px;
        }
    )");
    descLayout->addWidget(m_descriptionEdit);
    
    // 限制条件组
    QGroupBox *limitsGroup = new QGroupBox("限制条件", this);
    limitsGroup->setStyleSheet(basicGroup->styleSheet());
    
    QFormLayout *limitsLayout = new QFormLayout(limitsGroup);
    
    m_timeLimitSpin = new QSpinBox(this);
    m_timeLimitSpin->setRange(100, 10000);
    m_timeLimitSpin->setValue(1000);
    m_timeLimitSpin->setSuffix(" ms");
    m_timeLimitSpin->setStyleSheet(R"(
        QSpinBox {
            background-color: #2d2d2d;
            color: #e8e8e8;
            border: 1px solid #3a3a3a;
            border-radius: 4px;
            padding: 8px;
        }
    )");
    
    m_memoryLimitSpin = new QSpinBox(this);
    m_memoryLimitSpin->setRange(64, 1024);
    m_memoryLimitSpin->setValue(256);
    m_memoryLimitSpin->setSuffix(" MB");
    m_memoryLimitSpin->setStyleSheet(m_timeLimitSpin->styleSheet());
    
    limitsLayout->addRow("时间限制:", m_timeLimitSpin);
    limitsLayout->addRow("内存限制:", m_memoryLimitSpin);
    
    // 测试用例组
    QGroupBox *testCaseGroup = new QGroupBox("测试用例", this);
    testCaseGroup->setStyleSheet(basicGroup->styleSheet());
    
    QVBoxLayout *testCaseLayout = new QVBoxLayout(testCaseGroup);
    
    m_testCaseList = new QListWidget(this);
    m_testCaseList->setStyleSheet(R"(
        QListWidget {
            background-color: #2d2d2d;
            border: 1px solid #3a3a3a;
            border-radius: 4px;
        }
        QListWidget::item {
            background-color: transparent;
            padding: 4px;
        }
    )");
    m_testCaseList->setMinimumHeight(200);
    
    m_addTestCaseButton = new QPushButton("➕ 添加测试用例", this);
    m_addTestCaseButton->setStyleSheet(R"(
        QPushButton {
            background-color: #660000;
            color: white;
            border: none;
            border-radius: 4px;
            padding: 10px 20px;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #800000;
        }
    )");
    
    testCaseLayout->addWidget(m_testCaseList);
    testCaseLayout->addWidget(m_addTestCaseButton);
    
    // 添加所有组到内容布局
    contentLayout->addWidget(basicGroup);
    contentLayout->addWidget(descGroup);
    contentLayout->addWidget(limitsGroup);
    contentLayout->addWidget(testCaseGroup);
    contentLayout->addStretch();
    
    scrollArea->setWidget(contentWidget);
    mainLayout->addWidget(scrollArea);
    
    // 底部按钮
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    
    if (m_mode == ImportMode || m_mode == CreateMode) {
        m_importButton = new QPushButton("📁 从文件导入", this);
        m_importButton->setStyleSheet(R"(
            QPushButton {
                background-color: #3a3a3a;
                color: white;
                border: none;
                border-radius: 4px;
                padding: 10px 20px;
            }
            QPushButton:hover {
                background-color: #4a4a4a;
            }
        )");
        buttonLayout->addWidget(m_importButton);
    }
    
    buttonLayout->addStretch();
    
    m_cancelButton = new QPushButton("取消", this);
    m_cancelButton->setStyleSheet(R"(
        QPushButton {
            background-color: #3a3a3a;
            color: white;
            border: none;
            border-radius: 4px;
            padding: 10px 20px;
        }
        QPushButton:hover {
            background-color: #4a4a4a;
        }
    )");
    
    m_okButton = new QPushButton(m_mode == EditMode ? "保存" : "创建", this);
    m_okButton->setStyleSheet(R"(
        QPushButton {
            background-color: #660000;
            color: white;
            border: none;
            border-radius: 4px;
            padding: 10px 20px;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #800000;
        }
    )");
    
    buttonLayout->addWidget(m_cancelButton);
    buttonLayout->addWidget(m_okButton);
    
    mainLayout->addLayout(buttonLayout);
    
    // 设置对话框样式
    setStyleSheet(R"(
        QDialog {
            background-color: #242424;
        }
        QLabel {
            color: #e8e8e8;
        }
    )");
}

void QuestionEditorDialog::setupConnections()
{
    connect(m_addTestCaseButton, &QPushButton::clicked, this, &QuestionEditorDialog::onAddTestCase);
    
    if (m_importButton) {
        connect(m_importButton, &QPushButton::clicked, this, &QuestionEditorDialog::onImportFromFile);
    }
    
    connect(m_okButton, &QPushButton::clicked, this, &QuestionEditorDialog::onAccept);
    connect(m_cancelButton, &QPushButton::clicked, this, &QuestionEditorDialog::onCancel);
}

void QuestionEditorDialog::onAddTestCase()
{
    int index = m_testCaseList->count();
    TestCaseItem *item = new TestCaseItem(index, this);
    
    connect(item, &TestCaseItem::removeRequested, this, [this, item]() {
        for (int i = 0; i < m_testCaseList->count(); ++i) {
            QListWidgetItem *listItem = m_testCaseList->item(i);
            if (m_testCaseList->itemWidget(listItem) == item) {
                delete m_testCaseList->takeItem(i);
                updateTestCaseIndices();
                break;
            }
        }
    });
    
    QListWidgetItem *listItem = new QListWidgetItem(m_testCaseList);
    listItem->setSizeHint(item->sizeHint());
    m_testCaseList->setItemWidget(listItem, item);
}

void QuestionEditorDialog::onRemoveTestCase(int index)
{
    if (index >= 0 && index < m_testCaseList->count()) {
        delete m_testCaseList->takeItem(index);
        updateTestCaseIndices();
    }
}

void QuestionEditorDialog::updateTestCaseIndices()
{
    for (int i = 0; i < m_testCaseList->count(); ++i) {
        QListWidgetItem *listItem = m_testCaseList->item(i);
        TestCaseItem *item = qobject_cast<TestCaseItem*>(m_testCaseList->itemWidget(listItem));
        if (item) {
            item->findChild<QLabel*>()->setText(QString("测试用例 #%1").arg(i + 1));
        }
    }
}

void QuestionEditorDialog::onImportFromFile()
{
    QString filePath = QFileDialog::getOpenFileName(
        this,
        "选择题目文件",
        QString(),
        "JSON 文件 (*.json);;Markdown 文件 (*.md);;所有文件 (*.*)"
    );
    
    if (!filePath.isEmpty()) {
        importFromFile(filePath);
    }
}

void QuestionEditorDialog::importFromFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "错误", "无法打开文件");
        return;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    if (filePath.endsWith(".json", Qt::CaseInsensitive)) {
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isObject()) {
            Question question(doc.object());
            setQuestion(question);
        } else {
            QMessageBox::warning(this, "错误", "无效的 JSON 格式");
        }
    } else {
        // Markdown 格式，简单解析
        QString content = QString::fromUtf8(data);
        m_descriptionEdit->setPlainText(content);
        QMessageBox::information(this, "提示", "已导入题目描述，请手动填写其他信息");
    }
}

void QuestionEditorDialog::onAccept()
{
    if (!validateInput()) {
        return;
    }
    
    accept();
}

void QuestionEditorDialog::onCancel()
{
    reject();
}

bool QuestionEditorDialog::validateInput()
{
    if (m_titleEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "验证失败", "请输入题目标题");
        m_titleEdit->setFocus();
        return false;
    }
    
    if (m_descriptionEdit->toPlainText().trimmed().isEmpty()) {
        QMessageBox::warning(this, "验证失败", "请输入题目描述");
        m_descriptionEdit->setFocus();
        return false;
    }
    
    if (m_testCaseList->count() == 0) {
        QMessageBox::warning(this, "验证失败", "请至少添加一个测试用例");
        return false;
    }
    
    return true;
}

QString QuestionEditorDialog::generateQuestionId() const
{
    QString title = m_titleEdit->text();
    QString timestamp = QString::number(QDateTime::currentMSecsSinceEpoch());
    QString data = title + timestamp;
    
    QByteArray hash = QCryptographicHash::hash(data.toUtf8(), QCryptographicHash::Md5);
    return "custom_" + hash.toHex().left(16);
}

Question QuestionEditorDialog::getQuestion() const
{
    Question question;
    
    // 生成或使用现有ID
    if (m_mode == CreateMode) {
        question.setId(generateQuestionId());
    } else {
        question.setId(m_question.id());
    }
    
    // 基本信息
    question.setTitle(m_titleEdit->text().trimmed());
    
    // 难度转换
    Difficulty difficulty = Difficulty::Easy;
    QString diffText = m_difficultyCombo->currentText();
    if (diffText == "中等") {
        difficulty = Difficulty::Medium;
    } else if (diffText == "困难") {
        difficulty = Difficulty::Hard;
    }
    question.setDifficulty(difficulty);
    
    // 标签
    QString tagsText = m_tagsEdit->text();
    QStringList tags = tagsText.split(',', Qt::SkipEmptyParts);
    for (QString &tag : tags) {
        tag = tag.trimmed();
    }
    question.setTags(tags);
    
    // 描述（将时间和内存限制添加到描述中）
    QString description = m_descriptionEdit->toPlainText();
    description += QString("\n\n**限制条件：**\n- 时间限制：%1 ms\n- 内存限制：%2 MB")
        .arg(m_timeLimitSpin->value())
        .arg(m_memoryLimitSpin->value());
    question.setDescription(description);
    
    // 测试用例
    QVector<TestCase> testCases;
    for (int i = 0; i < m_testCaseList->count(); ++i) {
        QListWidgetItem *listItem = m_testCaseList->item(i);
        TestCaseItem *item = qobject_cast<TestCaseItem*>(m_testCaseList->itemWidget(listItem));
        if (item) {
            testCases.append(item->getTestCase());
        }
    }
    question.setTestCases(testCases);
    
    return question;
}

void QuestionEditorDialog::setQuestion(const Question &question)
{
    m_question = question;
    
    // 基本信息
    m_titleEdit->setText(question.title());
    
    // 难度转换
    QString diffText = "简单";
    switch (question.difficulty()) {
        case Difficulty::Easy:
            diffText = "简单";
            break;
        case Difficulty::Medium:
            diffText = "中等";
            break;
        case Difficulty::Hard:
            diffText = "困难";
            break;
    }
    int difficultyIndex = m_difficultyCombo->findText(diffText);
    if (difficultyIndex >= 0) {
        m_difficultyCombo->setCurrentIndex(difficultyIndex);
    }
    
    m_tagsEdit->setText(question.tags().join(", "));
    
    // 描述（尝试从描述中提取限制条件）
    QString description = question.description();
    
    // 尝试解析时间和内存限制
    QRegularExpression timeRegex("时间限制[：:](\\d+)\\s*ms");
    QRegularExpression memoryRegex("内存限制[：:](\\d+)\\s*MB");
    
    QRegularExpressionMatch timeMatch = timeRegex.match(description);
    if (timeMatch.hasMatch()) {
        m_timeLimitSpin->setValue(timeMatch.captured(1).toInt());
    }
    
    QRegularExpressionMatch memoryMatch = memoryRegex.match(description);
    if (memoryMatch.hasMatch()) {
        m_memoryLimitSpin->setValue(memoryMatch.captured(1).toInt());
    }
    
    m_descriptionEdit->setPlainText(description);
    
    // 测试用例
    m_testCaseList->clear();
    for (const TestCase &testCase : question.testCases()) {
        int index = m_testCaseList->count();
        TestCaseItem *item = new TestCaseItem(index, this);
        item->setTestCase(testCase);
        
        connect(item, &TestCaseItem::removeRequested, this, [this, item]() {
            for (int i = 0; i < m_testCaseList->count(); ++i) {
                QListWidgetItem *listItem = m_testCaseList->item(i);
                if (m_testCaseList->itemWidget(listItem) == item) {
                    delete m_testCaseList->takeItem(i);
                    updateTestCaseIndices();
                    break;
                }
            }
        });
        
        QListWidgetItem *listItem = new QListWidgetItem(m_testCaseList);
        listItem->setSizeHint(item->sizeHint());
        m_testCaseList->setItemWidget(listItem, item);
    }
}
