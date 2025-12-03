#include "PracticeWidget.h"
#include "../core/ProgressManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QSet>
#include <QDebug>

PracticeWidget::PracticeWidget(QuestionBank *questionBank, QWidget *parent)
    : QWidget(parent)
    , m_questionBank(questionBank)
    , m_currentDifficulty(Difficulty::Easy)
    , m_currentStatus(-1)
{
    setupUI();
    
    // 延迟加载，避免初始化时崩溃
    // loadQuestions() 和 updateStatistics() 会在 refreshQuestionList() 中调用
    
    // 连接进度管理器信号
    connect(&ProgressManager::instance(), &ProgressManager::statisticsChanged,
            this, &PracticeWidget::updateStatistics);
}

void PracticeWidget::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(16);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    
    // === 标题和统计信息 ===
    QHBoxLayout *headerLayout = new QHBoxLayout();
    
    QLabel *titleLabel = new QLabel("📚 刷题系统", this);
    titleLabel->setStyleSheet("font-size: 18pt; font-weight: bold; color: #e8e8e8;");
    
    m_statsLabel = new QLabel(this);
    m_statsLabel->setStyleSheet("color: #b0b0b0; font-size: 10pt;");
    
    m_progressLabel = new QLabel(this);
    m_progressLabel->setStyleSheet("color: #e8e8e8; font-size: 10pt; font-weight: bold;");
    
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    headerLayout->addWidget(m_statsLabel);
    headerLayout->addWidget(m_progressLabel);
    
    // === 搜索和筛选 ===
    QHBoxLayout *filterLayout = new QHBoxLayout();
    
    // 搜索框
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText("🔍 搜索题目...");
    m_searchEdit->setFixedWidth(250);
    
    // 难度筛选
    m_difficultyFilter = new QComboBox(this);
    m_difficultyFilter->addItem("全部难度", -1);
    m_difficultyFilter->addItem("简单", static_cast<int>(Difficulty::Easy));
    m_difficultyFilter->addItem("中等", static_cast<int>(Difficulty::Medium));
    m_difficultyFilter->addItem("困难", static_cast<int>(Difficulty::Hard));
    
    // 题型筛选
    m_tagFilter = new QComboBox(this);
    m_tagFilter->addItem("全部题型");
    
    // 状态筛选
    m_statusFilter = new QComboBox(this);
    m_statusFilter->addItem("全部状态", -1);
    m_statusFilter->addItem("❌ 未开始", 0);
    m_statusFilter->addItem("⏳ 进行中", 1);
    m_statusFilter->addItem("✅ 已完成", 2);
    m_statusFilter->addItem("⭐ 已掌握", 3);
    
    // 刷新按钮
    m_refreshBtn = new QPushButton("🔄 刷新", this);
    m_resetProgressBtn = new QPushButton("🗑️ 重置进度", this);
    
    QString btnStyle = R"(
        QPushButton {
            background-color: #660000;
            color: white;
            border: none;
            border-radius: 8px;
            padding: 8px 16px;
            font-weight: 500;
        }
        QPushButton:hover {
            background-color: #880000;
        }
        QPushButton:pressed {
            background-color: #440000;
        }
    )";
    
    m_refreshBtn->setStyleSheet(btnStyle);
    m_resetProgressBtn->setStyleSheet(btnStyle);
    
    filterLayout->addWidget(m_searchEdit);
    filterLayout->addWidget(m_difficultyFilter);
    filterLayout->addWidget(m_tagFilter);
    filterLayout->addWidget(m_statusFilter);
    filterLayout->addStretch();
    filterLayout->addWidget(m_refreshBtn);
    filterLayout->addWidget(m_resetProgressBtn);
    
    // === 题目列表表格 ===
    m_questionTable = new QTableWidget(this);
    m_questionTable->setColumnCount(7);
    m_questionTable->setHorizontalHeaderLabels({
        "状态", "题号", "题目", "难度", "题型", "正确率", "尝试次数"
    });
    
    m_questionTable->horizontalHeader()->setStretchLastSection(false);
    m_questionTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_questionTable->setColumnWidth(0, 80);
    m_questionTable->setColumnWidth(1, 80);
    m_questionTable->setColumnWidth(3, 80);
    m_questionTable->setColumnWidth(4, 150);
    m_questionTable->setColumnWidth(5, 100);
    m_questionTable->setColumnWidth(6, 100);
    
    m_questionTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_questionTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_questionTable->setAlternatingRowColors(true);
    
    m_questionTable->setStyleSheet(R"(
        QTableWidget {
            background-color: #242424;
            color: #e8e8e8;
            border: 1px solid #4a4a4a;
            border-radius: 12px;
            gridline-color: #4a4a4a;
        }
        QTableWidget::item {
            padding: 8px;
        }
        QTableWidget::item:selected {
            background-color: #660000;
        }
        QHeaderView::section {
            background-color: #242424;
            color: #e8e8e8;
            padding: 10px;
            border: none;
            font-weight: bold;
        }
    )");
    
    mainLayout->addLayout(headerLayout);
    mainLayout->addLayout(filterLayout);
    mainLayout->addWidget(m_questionTable);
    
    // 连接信号
    connect(m_searchEdit, &QLineEdit::textChanged, this, &PracticeWidget::onSearchTextChanged);
    connect(m_difficultyFilter, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PracticeWidget::onFilterChanged);
    connect(m_tagFilter, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PracticeWidget::onFilterChanged);
    connect(m_statusFilter, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PracticeWidget::onFilterChanged);
    connect(m_questionTable, &QTableWidget::cellDoubleClicked,
            this, &PracticeWidget::onQuestionDoubleClicked);
    connect(m_refreshBtn, &QPushButton::clicked, this, &PracticeWidget::onRefreshClicked);
    connect(m_resetProgressBtn, &QPushButton::clicked, this, &PracticeWidget::onResetProgressClicked);
}

void PracticeWidget::loadQuestions()
{
    qDebug() << "[PracticeWidget] loadQuestions() started";
    
    try {
        m_questionTable->setRowCount(0);
        qDebug() << "[PracticeWidget] Table cleared";
    } catch (...) {
        qCritical() << "[PracticeWidget] Failed to clear table";
        return;
    }
    
    if (!m_questionBank) {
        qWarning() << "[PracticeWidget] QuestionBank is null";
        return;
    }
    
    int count = 0;
    try {
        count = m_questionBank->count();
        qDebug() << "[PracticeWidget] QuestionBank count:" << count;
    } catch (...) {
        qCritical() << "[PracticeWidget] Exception getting count";
        return;
    }
    
    if (count == 0) {
        qDebug() << "[PracticeWidget] QuestionBank is empty";
        return;
    }
    
    // 安全获取题目列表
    QVector<Question> allQuestions;
    try {
        allQuestions = m_questionBank->allQuestions();
        qDebug() << "[PracticeWidget] Got questions, size:" << allQuestions.size();
    } catch (...) {
        qCritical() << "[PracticeWidget] Exception when getting questions from bank";
        return;
    }
    
    if (allQuestions.isEmpty()) {
        qDebug() << "[PracticeWidget] Question list is empty";
        return;
    }
    
    // 收集所有题型标签
    QSet<QString> allTags;
    try {
        qDebug() << "[PracticeWidget] Collecting tags...";
        for (const auto &q : allQuestions) {
            for (const auto &tag : q.tags()) {
                allTags.insert(tag);
            }
        }
        qDebug() << "[PracticeWidget] Tags collected:" << allTags.size();
    } catch (...) {
        qCritical() << "[PracticeWidget] Exception collecting tags";
        return;
    }
    
    // 更新题型筛选下拉框
    try {
        qDebug() << "[PracticeWidget] Updating tag filter...";
        if (!m_tagFilter) {
            qCritical() << "[PracticeWidget] m_tagFilter is null!";
            return;
        }
        
        // 临时断开信号，避免触发无限循环
        m_tagFilter->blockSignals(true);
        
        QString currentTag = m_tagFilter->currentText();
        m_tagFilter->clear();
        m_tagFilter->addItem("全部题型");
        for (const auto &tag : allTags) {
            m_tagFilter->addItem(tag);
        }
        if (!currentTag.isEmpty()) {
            int index = m_tagFilter->findText(currentTag);
            if (index >= 0) {
                m_tagFilter->setCurrentIndex(index);
            }
        }
        
        // 恢复信号
        m_tagFilter->blockSignals(false);
        
        qDebug() << "[PracticeWidget] Tag filter updated";
    } catch (...) {
        qCritical() << "[PracticeWidget] Exception updating tag filter";
        m_tagFilter->blockSignals(false);  // 确保恢复信号
        return;
    }
    
    // 获取筛选条件
    int difficultyIndex = -1;
    QString selectedTag = "全部题型";
    int statusFilter = -1;
    
    try {
        if (m_difficultyFilter) {
            difficultyIndex = m_difficultyFilter->currentData().toInt();
        }
        if (m_tagFilter) {
            selectedTag = m_tagFilter->currentText();
        }
        if (m_statusFilter) {
            statusFilter = m_statusFilter->currentData().toInt();
        }
        qDebug() << "[PracticeWidget] Filters:" << difficultyIndex << selectedTag << statusFilter;
    } catch (...) {
        qCritical() << "[PracticeWidget] Exception getting filter values";
        return;
    }
    
    // 加载题目
    qDebug() << "[PracticeWidget] Loading questions into table...";
    int displayIndex = 1;
    int loadedCount = 0;
    
    try {
        for (const auto &question : allQuestions) {
            loadedCount++;
        // 应用筛选
        if (difficultyIndex >= 0 && question.difficulty() != static_cast<Difficulty>(difficultyIndex)) {
            continue;
        }
        
        if (selectedTag != "全部题型" && !question.tags().contains(selectedTag)) {
            continue;
        }
        
        if (!m_currentSearchText.isEmpty() && 
            !question.title().contains(m_currentSearchText, Qt::CaseInsensitive)) {
            continue;
        }
        
        // 获取进度信息
        QuestionProgressRecord progress = ProgressManager::instance().getProgress(question.id());
        
        if (statusFilter >= 0 && static_cast<int>(progress.status) != statusFilter) {
            continue;
        }
        
        int row = m_questionTable->rowCount();
        m_questionTable->insertRow(row);
        
        // 状态
        QString statusIcon = getStatusIcon(question.id());
        QTableWidgetItem *statusItem = new QTableWidgetItem(statusIcon);
        statusItem->setTextAlignment(Qt::AlignCenter);
        m_questionTable->setItem(row, 0, statusItem);
        
        // 题号
        QTableWidgetItem *indexItem = new QTableWidgetItem(QString::number(displayIndex++));
        indexItem->setTextAlignment(Qt::AlignCenter);
        indexItem->setData(Qt::UserRole, question.id());
        m_questionTable->setItem(row, 1, indexItem);
        
        // 题目
        m_questionTable->setItem(row, 2, new QTableWidgetItem(question.title()));
        
        // 难度
        QString diffText;
        QString diffColor;
        switch (question.difficulty()) {
            case Difficulty::Easy:
                diffText = "简单";
                diffColor = "#e8e8e8";
                break;
            case Difficulty::Medium:
                diffText = "中等";
                diffColor = "#b0b0b0";
                break;
            case Difficulty::Hard:
                diffText = "困难";
                diffColor = "#660000";
                break;
        }
        QTableWidgetItem *diffItem = new QTableWidgetItem(diffText);
        diffItem->setForeground(QColor(diffColor));
        diffItem->setTextAlignment(Qt::AlignCenter);
        m_questionTable->setItem(row, 3, diffItem);
        
        // 题型
        QString tagsText = question.tags().join(", ");
        m_questionTable->setItem(row, 4, new QTableWidgetItem(tagsText));
        
        // 正确率
        QString accuracyText = progress.attemptCount > 0 
            ? QString("%1%").arg(progress.accuracy(), 0, 'f', 1)
            : "-";
        QTableWidgetItem *accuracyItem = new QTableWidgetItem(accuracyText);
        accuracyItem->setTextAlignment(Qt::AlignCenter);
        m_questionTable->setItem(row, 5, accuracyItem);
        
        // 尝试次数
        QString attemptText = progress.attemptCount > 0 
            ? QString::number(progress.attemptCount)
            : "-";
        QTableWidgetItem *attemptItem = new QTableWidgetItem(attemptText);
        attemptItem->setTextAlignment(Qt::AlignCenter);
        m_questionTable->setItem(row, 6, attemptItem);
    }
    
    qDebug() << "[PracticeWidget] loadQuestions() completed. Processed:" << loadedCount << "Displayed:" << m_questionTable->rowCount();
    
    } catch (const std::exception &e) {
        qCritical() << "[PracticeWidget] Exception in loadQuestions loop:" << e.what();
    } catch (...) {
        qCritical() << "[PracticeWidget] Unknown exception in loadQuestions loop";
    }
}

void PracticeWidget::updateStatistics()
{
    if (!m_questionBank) {
        m_statsLabel->setText("题库未加载");
        m_progressLabel->setText("进度: 0%");
        return;
    }
    
    int total = 0;
    try {
        total = m_questionBank->count();
    } catch (...) {
        qWarning() << "Exception when getting question count";
        m_statsLabel->setText("题库错误");
        m_progressLabel->setText("进度: 0%");
        return;
    }
    
    if (total == 0) {
        m_statsLabel->setText("题库为空");
        m_progressLabel->setText("进度: 0%");
        return;
    }
    int completed = ProgressManager::instance().getCompletedCount();
    int mastered = ProgressManager::instance().getMasteredCount();
    double accuracy = ProgressManager::instance().getOverallAccuracy();
    
    m_statsLabel->setText(QString("总题数: %1 | 已完成: %2 | 已掌握: %3 | 总正确率: %4%")
        .arg(total)
        .arg(completed)
        .arg(mastered)
        .arg(accuracy, 0, 'f', 1));
    
    // 进度条文本
    int percentage = total > 0 ? (completed * 100 / total) : 0;
    m_progressLabel->setText(QString("进度: %1%").arg(percentage));
}

QString PracticeWidget::getStatusIcon(const QString &questionId) const
{
    QuestionProgressRecord progress = ProgressManager::instance().getProgress(questionId);
    
    switch (progress.status) {
        case QuestionStatus::NotStarted:
            return "❌";
        case QuestionStatus::InProgress:
            return "⏳";
        case QuestionStatus::Completed:
            return "✅";
        case QuestionStatus::Mastered:
            return "⭐";
        default:
            return "❓";
    }
}

QString PracticeWidget::getStatusText(const QString &questionId) const
{
    QuestionProgressRecord progress = ProgressManager::instance().getProgress(questionId);
    
    switch (progress.status) {
        case QuestionStatus::NotStarted:
            return "未开始";
        case QuestionStatus::InProgress:
            return "进行中";
        case QuestionStatus::Completed:
            return "已完成";
        case QuestionStatus::Mastered:
            return "已掌握";
        default:
            return "未知";
    }
}

void PracticeWidget::onFilterChanged()
{
    loadQuestions();
}

void PracticeWidget::onSearchTextChanged(const QString &text)
{
    m_currentSearchText = text;
    loadQuestions();
}

void PracticeWidget::onQuestionDoubleClicked(int row, int column)
{
    Q_UNUSED(column);
    
    QTableWidgetItem *item = m_questionTable->item(row, 1);
    if (!item) return;
    
    QString questionId = item->data(Qt::UserRole).toString();
    Question question = m_questionBank->getQuestion(questionId);
    
    if (!question.id().isEmpty()) {
        emit questionSelected(question);
    }
}

void PracticeWidget::onRefreshClicked()
{
    loadQuestions();
    updateStatistics();
}

void PracticeWidget::onResetProgressClicked()
{
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "确认重置",
        "确定要重置所有刷题进度吗？此操作不可恢复。",
        QMessageBox::Yes | QMessageBox::No
    );
    
    if (reply == QMessageBox::Yes) {
        ProgressManager::instance().clear();
        loadQuestions();
        updateStatistics();
        QMessageBox::information(this, "完成", "刷题进度已重置");
    }
}

void PracticeWidget::refreshQuestionList()
{
    // 安全检查
    if (!m_questionBank) {
        qWarning() << "QuestionBank is null in refreshQuestionList()";
        m_statsLabel->setText("题库未加载");
        m_progressLabel->setText("进度: 0%");
        return;
    }
    
    loadQuestions();
    updateStatistics();
}
