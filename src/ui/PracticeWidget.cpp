#include "PracticeWidget.h"
#include "PracticeStatsPanel.h"
#include "../core/ProgressManager.h"
#include "../core/QuestionBankManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QInputDialog>
#include <QFileDialog>
#include <QTextStream>
#include <QStringConverter>
#include <QDateTime>
#include <QRandomGenerator>
#include <QSet>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QScrollArea>
#include <QScrollBar>
#include <QEvent>
#include <QWheelEvent>
#include <algorithm>

PracticeWidget::PracticeWidget(QuestionBank *questionBank, QWidget *parent)
    : QWidget(parent)
    , m_questionBank(questionBank)
    , m_currentDifficulty(Difficulty::Easy)
    , m_currentStatus(-1)
    , m_sortColumn(-1)
    , m_sortOrder(Qt::AscendingOrder)
{
    setupUI();
    
    // 延迟加载，避免初始化时崩溃
    // loadQuestions() 和 updateStatistics() 会在 refreshQuestionList() 中调用
    
    // 连接进度管理器信号
    connect(&ProgressManager::instance(), &ProgressManager::statisticsChanged,
            this, &PracticeWidget::updateStatistics);
    connect(&ProgressManager::instance(), &ProgressManager::progressUpdated,
            this, &PracticeWidget::onQuestionStatusUpdated);
}

void PracticeWidget::setupUI()
{
    // 创建主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    
    // 创建滚动区域
    QScrollArea *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    
    // 创建内容容器
    QWidget *contentWidget = new QWidget();
    QVBoxLayout *contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setSpacing(16);
    contentLayout->setContentsMargins(16, 16, 16, 16);
    
    // === 标题和题库选择器 ===
    QHBoxLayout *headerLayout = new QHBoxLayout();
    
    QLabel *titleLabel = new QLabel("📚 题库面板", this);
    titleLabel->setStyleSheet("font-size: 18pt; font-weight: bold; color: #e8e8e8;");
    
    // 题库选择器 (10pt字体 * 1.5 = 15pt等效高度，需要约54px)
    m_bankSelector = new QComboBox(this);
    m_bankSelector->setMinimumWidth(200);
    m_bankSelector->setMinimumHeight(54);
    m_bankSelector->setStyleSheet(
        "QComboBox {"
        "    background-color: #2d2d2d;"
        "    color: #e8e8e8;"
        "    border: 1px solid #4a4a4a;"
        "    border-radius: 8px;"
        "    padding: 10px 16px;"
        "    font-size: 10pt;"
        "}"
        "QComboBox:hover {"
        "    border-color: #660000;"
        "}"
        "QComboBox::drop-down {"
        "    border: none;"
        "}"
        "QComboBox QAbstractItemView {"
        "    background-color: #2d2d2d;"
        "    color: #e8e8e8;"
        "    selection-background-color: #660000;"
        "    outline: 0px;"
        "}"
    );
    
    QPushButton *switchBankBtn = new QPushButton("切换题库", this);
    switchBankBtn->setMinimumHeight(54);
    switchBankBtn->setStyleSheet(
        "QPushButton {"
        "    background-color: #660000;"
        "    color: white;"
        "    border: none;"
        "    border-radius: 8px;"
        "    padding: 12px 20px;"
        "    font-weight: 500;"
        "}"
        "QPushButton:hover {"
        "    background-color: #880000;"
        "}"
    );
    connect(switchBankBtn, &QPushButton::clicked, this, &PracticeWidget::onSwitchBankClicked);
    
    headerLayout->addWidget(titleLabel);
    headerLayout->addSpacing(20);
    headerLayout->addWidget(new QLabel("当前题库:", this));
    headerLayout->addWidget(m_bankSelector);
    headerLayout->addWidget(switchBankBtn);
    headerLayout->addStretch();
    
    // === 统计信息和进度条 ===
    QHBoxLayout *statsLayout = new QHBoxLayout();
    
    m_statsLabel = new QLabel(this);
    m_statsLabel->setStyleSheet("color: #b0b0b0; font-size: 10pt;");
    
    m_progressBar = new QProgressBar(this);
    m_progressBar->setFixedWidth(200);
    m_progressBar->setFixedHeight(20);
    m_progressBar->setStyleSheet(
        "QProgressBar {"
        "    border: 1px solid #4a4a4a;"
        "    border-radius: 0px;"
        "    background-color: #2d2d2d;"
        "    text-align: center;"
        "    color: #e8e8e8;"
        "}"
        "QProgressBar::chunk {"
        "    background-color: #660000;"
        "    border-radius: 0px;"
        "}"
    );
    
    statsLayout->addWidget(m_statsLabel);
    statsLayout->addStretch();
    statsLayout->addWidget(m_progressBar);
    
    // === 搜索和筛选 ===
    QHBoxLayout *filterLayout = new QHBoxLayout();
    
    // 搜索框 (10pt字体 * 1.5 = 15pt等效高度，需要约54px)
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText("🔍 搜索题目...");
    m_searchEdit->setFixedWidth(250);
    m_searchEdit->setMinimumHeight(54);
    m_searchEdit->setStyleSheet(
        "QLineEdit {"
        "    background-color: #2d2d2d;"
        "    color: #e8e8e8;"
        "    border: 1px solid #4a4a4a;"
        "    border-radius: 8px;"
        "    padding: 12px 16px;"
        "}"
        "QLineEdit:focus {"
        "    border-color: #660000;"
        "}"
    );
    
    // 难度筛选
    m_difficultyFilter = new QComboBox(this);
    m_difficultyFilter->addItem("全部难度", -1);
    m_difficultyFilter->addItem("简单", static_cast<int>(Difficulty::Easy));
    m_difficultyFilter->addItem("中等", static_cast<int>(Difficulty::Medium));
    m_difficultyFilter->addItem("困难", static_cast<int>(Difficulty::Hard));
    m_difficultyFilter->setFocusPolicy(Qt::StrongFocus);
    m_difficultyFilter->installEventFilter(this);  // 问题1：禁止滚轮改变选项
    
    // 题型筛选
    m_tagFilter = new QComboBox(this);
    m_tagFilter->addItem("全部题型");
    m_tagFilter->setFocusPolicy(Qt::StrongFocus);
    m_tagFilter->installEventFilter(this);  // 问题1：禁止滚轮改变选项
    
    // 状态筛选
    m_statusFilter = new QComboBox(this);
    m_statusFilter->addItem("全部状态", -1);
    m_statusFilter->addItem("❌ 未开始", 0);
    m_statusFilter->addItem("⏳ 进行中", 1);
    m_statusFilter->addItem("✅ 已完成", 2);
    m_statusFilter->addItem("⭐ 已掌握", 3);
    m_statusFilter->addItem("🤖 AI判题通过", 4);
    m_statusFilter->setFocusPolicy(Qt::StrongFocus);
    m_statusFilter->installEventFilter(this);  // 问题1：禁止滚轮改变选项
    
    // 排序选择
    m_sortCombo = new QComboBox(this);
    m_sortCombo->addItem("默认排序", -1);
    m_sortCombo->addItem("按题号", 1);
    m_sortCombo->addItem("按难度", 3);
    m_sortCombo->addItem("按正确率", 5);
    m_sortCombo->addItem("按尝试次数", 6);
    m_sortCombo->setFocusPolicy(Qt::StrongFocus);
    m_sortCombo->installEventFilter(this);  // 问题1：禁止滚轮改变选项
    
    QString comboStyle = 
        "QComboBox {"
        "    background-color: #2d2d2d;"
        "    color: #e8e8e8;"
        "    border: 1px solid #4a4a4a;"
        "    border-radius: 8px;"
        "    padding: 12px 16px;"
        "}"
        "QComboBox:hover {"
        "    border-color: #660000;"
        "}"
        "QComboBox QAbstractItemView {"
        "    background-color: #2d2d2d;"
        "    color: #e8e8e8;"
        "    selection-background-color: #660000;"
        "    padding: 6px;"
        "}";
    
    // 10pt字体 * 1.5 = 15pt等效高度，需要约54px
    m_difficultyFilter->setMinimumHeight(54);
    m_tagFilter->setMinimumHeight(54);
    m_statusFilter->setMinimumHeight(54);
    m_sortCombo->setMinimumHeight(54);
    
    m_difficultyFilter->setStyleSheet(comboStyle);
    m_tagFilter->setStyleSheet(comboStyle);
    m_statusFilter->setStyleSheet(comboStyle);
    m_sortCombo->setStyleSheet(comboStyle);
    
    filterLayout->addWidget(m_searchEdit);
    filterLayout->addWidget(m_difficultyFilter);
    filterLayout->addWidget(m_tagFilter);
    filterLayout->addWidget(m_statusFilter);
    filterLayout->addWidget(m_sortCombo);
    filterLayout->addStretch();
    
    // === 快捷操作按钮 ===
    QHBoxLayout *actionLayout = new QHBoxLayout();
    
    m_randomBtn = new QPushButton("🎲 随机题目", this);
    m_recommendBtn = new QPushButton("💡 推荐题目", this);
    m_batchMarkBtn = new QPushButton("✓ 批量标记", this);
    m_exportBtn = new QPushButton("📊 导出报告", this);
    m_refreshBtn = new QPushButton("🔄 刷新", this);
    m_resetProgressBtn = new QPushButton("🗑️ 重置进度", this);
    
    QString btnStyle = 
        "QPushButton {"
        "    background-color: #660000;"
        "    color: white;"
        "    border: none;"
        "    border-radius: 10px;"
        "    padding: 14px 20px;"
        "    font-weight: 500;"
        "}"
        "QPushButton:hover {"
        "    background-color: #880000;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #440000;"
        "}";
    
    // 10pt字体 * 1.5 = 15pt等效高度，需要约56px
    m_randomBtn->setMinimumHeight(56);
    m_recommendBtn->setMinimumHeight(56);
    m_batchMarkBtn->setMinimumHeight(56);
    m_exportBtn->setMinimumHeight(56);
    m_refreshBtn->setMinimumHeight(56);
    m_resetProgressBtn->setMinimumHeight(56);
    
    m_randomBtn->setStyleSheet(btnStyle);
    m_recommendBtn->setStyleSheet(btnStyle);
    m_batchMarkBtn->setStyleSheet(btnStyle);
    m_exportBtn->setStyleSheet(btnStyle);
    m_refreshBtn->setStyleSheet(btnStyle);
    m_resetProgressBtn->setStyleSheet(btnStyle);
    
    actionLayout->addWidget(m_randomBtn);
    actionLayout->addWidget(m_recommendBtn);
    actionLayout->addWidget(m_batchMarkBtn);
    actionLayout->addWidget(m_exportBtn);
    actionLayout->addStretch();
    actionLayout->addWidget(m_refreshBtn);
    actionLayout->addWidget(m_resetProgressBtn);
    
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
    m_questionTable->setSelectionMode(QAbstractItemView::ExtendedSelection);  // 支持多选
    m_questionTable->setAlternatingRowColors(true);
    m_questionTable->setSortingEnabled(false);  // 手动控制排序
    m_questionTable->setFocusPolicy(Qt::StrongFocus);
    
    // 问题2：安装事件过滤器，阻止表格的滚轮事件传递给父容器
    m_questionTable->installEventFilter(this);
    m_questionTable->viewport()->installEventFilter(this);
    
    // 设置最小高度，确保至少显示10行
    // 行高约40px（padding 8px * 2 + 文字高度约24px）
    // 表头高度约40px
    m_questionTable->setMinimumHeight(440);  // 10行 * 40px + 表头40px
    
    m_questionTable->setStyleSheet(
        "QTableWidget {"
        "    background-color: #242424;"
        "    color: #e8e8e8;"
        "    border: 1px solid #4a4a4a;"
        "    border-radius: 12px;"
        "    gridline-color: #4a4a4a;"
        "    selection-background-color: #660000;"
        "    selection-color: #ffffff;"
        "    outline: 0;"
        "}"
        "QTableWidget::item {"
        "    padding: 8px;"
        "    border: none;"
        "    outline: none;"
        "}"
        "QTableWidget::item:selected {"
        "    background-color: #660000;"
        "    color: #ffffff;"
        "    border: none;"
        "    outline: none;"
        "}"
        "QTableWidget::item:selected:hover {"
        "    background-color: #880000;"
        "    color: #ffffff;"
        "}"
        "QTableWidget::item:focus {"
        "    background-color: #660000;"
        "    color: #ffffff;"
        "    outline: none;"
        "    border: none;"
        "}"
        "QTableWidget::item:selected:focus {"
        "    background-color: #660000;"
        "    color: #ffffff;"
        "    outline: none;"
        "    border: none;"
        "}"
        "QHeaderView::section {"
        "    background-color: #2d2d2d;"
        "    color: #e8e8e8;"
        "    padding: 10px;"
        "    border: none;"
        "    font-weight: bold;"
        "}"
        "QHeaderView::section:hover {"
        "    background-color: #3a3a3a;"
        "    cursor: pointer;"
        "}"
    );
    
    // === 刷题统计面板 ===
    m_statsPanel = new PracticeStatsPanel(contentWidget);
    // 移除最大高度限制，让内容自然展开
    
    contentLayout->addLayout(headerLayout);
    contentLayout->addLayout(statsLayout);
    contentLayout->addWidget(m_statsPanel);  // 添加统计面板
    contentLayout->addLayout(filterLayout);
    contentLayout->addLayout(actionLayout);
    contentLayout->addWidget(m_questionTable);
    
    // 设置滚动区域的内容
    scrollArea->setWidget(contentWidget);
    
    // 将滚动区域添加到主布局
    mainLayout->addWidget(scrollArea);
    
    // 连接信号
    connect(m_searchEdit, &QLineEdit::textChanged, this, &PracticeWidget::onSearchTextChanged);
    connect(m_difficultyFilter, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PracticeWidget::onFilterChanged);
    connect(m_tagFilter, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PracticeWidget::onFilterChanged);
    connect(m_statusFilter, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PracticeWidget::onFilterChanged);
    connect(m_sortCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PracticeWidget::onFilterChanged);
    connect(m_questionTable, &QTableWidget::cellDoubleClicked,
            this, &PracticeWidget::onQuestionDoubleClicked);
    connect(m_questionTable->horizontalHeader(), &QHeaderView::sectionClicked,
            this, &PracticeWidget::onHeaderClicked);
    connect(m_randomBtn, &QPushButton::clicked, this, &PracticeWidget::onRandomQuestionClicked);
    connect(m_recommendBtn, &QPushButton::clicked, this, &PracticeWidget::onRecommendQuestionClicked);
    connect(m_batchMarkBtn, &QPushButton::clicked, this, &PracticeWidget::onBatchMarkClicked);
    connect(m_exportBtn, &QPushButton::clicked, this, &PracticeWidget::onExportProgressClicked);
    connect(m_refreshBtn, &QPushButton::clicked, this, &PracticeWidget::onRefreshClicked);
    connect(m_resetProgressBtn, &QPushButton::clicked, this, &PracticeWidget::onResetProgressClicked);
    
    // 更新题库选择器
    updateBankSelector();
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
    
    // 从当前选中的题库加载题目
    QString currentBankId = QuestionBankManager::instance().getCurrentBankId();
    if (currentBankId.isEmpty()) {
        qWarning() << "[PracticeWidget] No current bank selected";
        return;
    }
    
    QuestionBankInfo bankInfo = QuestionBankManager::instance().getBankInfo(currentBankId);
    if (bankInfo.id.isEmpty()) {
        qWarning() << "[PracticeWidget] Bank info not found for:" << currentBankId;
        return;
    }
    
    qDebug() << "[PracticeWidget] Loading from bank:" << bankInfo.name << "path:" << bankInfo.path;
    
    // 从题库路径加载所有题目
    QVector<Question> allQuestions = loadQuestionsFromBank(bankInfo.path);
    qDebug() << "[PracticeWidget] Loaded questions:" << allQuestions.size();
    
    if (allQuestions.isEmpty()) {
        qDebug() << "[PracticeWidget] No questions found in bank";
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
        
        // 状态筛选
        if (statusFilter >= 0) {
            if (statusFilter == 4) {
                // AI判题通过筛选
                if (!progress.aiJudgePassed) {
                    continue;
                }
            } else if (static_cast<int>(progress.status) != statusFilter) {
                continue;
            }
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
    // 从当前选中的题库获取题目
    QString currentBankId = QuestionBankManager::instance().getCurrentBankId();
    if (currentBankId.isEmpty()) {
        m_statsLabel->setText("未选择题库");
        m_progressBar->setValue(0);
        return;
    }
    
    QuestionBankInfo bankInfo = QuestionBankManager::instance().getBankInfo(currentBankId);
    if (bankInfo.id.isEmpty()) {
        m_statsLabel->setText("题库信息错误");
        m_progressBar->setValue(0);
        return;
    }
    
    // 加载题目
    QVector<Question> allQuestions = loadQuestionsFromBank(bankInfo.path);
    int total = allQuestions.size();
    
    if (total == 0) {
        m_statsLabel->setText("题库为空");
        m_progressBar->setValue(0);
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
    
    // 更新进度条
    int percentage = total > 0 ? (completed * 100 / total) : 0;
    m_progressBar->setValue(percentage);
    m_progressBar->setFormat(QString("完成进度: %1%").arg(percentage));
    
    // 更新统计面板
    if (m_statsPanel) {
        ProgressManager &pm = ProgressManager::instance();
        
        // 更新统计卡片
        m_statsPanel->updateStats(
            pm.getTotalCompleted(),
            pm.getCurrentStreak(),
            pm.getLongestStreak(),
            pm.getTodayCompleted()
        );
        
        // 更新热力图
        QMap<QDate, int> activityData = pm.getActivityByDate(84);  // 最近12周
        m_statsPanel->updateHeatMap(activityData);
        
        // 更新难度分布（需要从题库获取）
        int easyCompleted = 0, mediumCompleted = 0, hardCompleted = 0;
        int easyTotal = 0, mediumTotal = 0, hardTotal = 0;
        
        for (const Question &q : allQuestions) {
            QuestionProgressRecord progress = pm.getProgress(q.id());
            
            switch (q.difficulty()) {
                case Difficulty::Easy:
                    easyTotal++;
                    if (progress.status == QuestionStatus::Completed || 
                        progress.status == QuestionStatus::Mastered) {
                        easyCompleted++;
                    }
                    break;
                case Difficulty::Medium:
                    mediumTotal++;
                    if (progress.status == QuestionStatus::Completed || 
                        progress.status == QuestionStatus::Mastered) {
                        mediumCompleted++;
                    }
                    break;
                case Difficulty::Hard:
                    hardTotal++;
                    if (progress.status == QuestionStatus::Completed || 
                        progress.status == QuestionStatus::Mastered) {
                        hardCompleted++;
                    }
                    break;
            }
        }
        
        m_statsPanel->updateDifficultyDistribution(
            easyCompleted, easyTotal,
            mediumCompleted, mediumTotal,
            hardCompleted, hardTotal
        );
    }
}

void PracticeWidget::updateBankSelector()
{
    qDebug() << "[PracticeWidget] updateBankSelector() started";
    
    m_bankSelector->clear();
    
    // 从 QuestionBankManager 获取所有题库
    QVector<QuestionBankInfo> banks = QuestionBankManager::instance().getAllBanks();
    QString currentBankId = QuestionBankManager::instance().getCurrentBankId();
    
    qDebug() << "[PracticeWidget] Found" << banks.size() << "banks, current:" << currentBankId;
    
    if (banks.isEmpty()) {
        m_bankSelector->addItem("暂无题库");
        m_bankSelector->setEnabled(false);
        qDebug() << "[PracticeWidget] No banks available";
        return;
    }
    
    m_bankSelector->setEnabled(true);
    int currentIndex = 0;
    
    // 如果没有当前题库，自动选择第一个
    if (currentBankId.isEmpty() && !banks.isEmpty()) {
        currentBankId = banks[0].id;
        QuestionBankManager::instance().switchToBank(currentBankId);
        qDebug() << "[PracticeWidget] Auto-selected first bank:" << banks[0].name;
    }
    
    for (int i = 0; i < banks.size(); ++i) {
        const QuestionBankInfo &info = banks[i];
        
        // 计算完成度
        int completedCount = 0;
        if (info.questionCount > 0) {
            // 加载题库中的所有题目并统计完成度
            QVector<Question> questions = loadQuestionsFromBank(info.path);
            for (const Question &q : questions) {
                QuestionProgressRecord record = ProgressManager::instance().getProgress(q.id());
                if (record.status == QuestionStatus::Completed || 
                    record.status == QuestionStatus::Mastered) {
                    completedCount++;
                }
            }
        }
        
        // 计算完成度百分比
        double completionRate = info.questionCount > 0 ? 
            (double)completedCount / info.questionCount * 100.0 : 0.0;
        
        // 显示格式：题库名称 (50题 | 68.0%)
        QString displayText = QString("%1 (%2题 | %3%)")
            .arg(info.name)
            .arg(info.questionCount)
            .arg(completionRate, 0, 'f', 1);
        
        m_bankSelector->addItem(displayText, info.id);
        
        if (info.id == currentBankId) {
            currentIndex = i;
        }
    }
    
    m_bankSelector->setCurrentIndex(currentIndex);
    qDebug() << "[PracticeWidget] Bank selector updated, selected index:" << currentIndex;
}

QVector<Question> PracticeWidget::loadQuestionsFromBank(const QString &bankPath) const
{
    QVector<Question> questions;
    
    QDir dir(bankPath);
    if (!dir.exists()) {
        return questions;
    }
    
    // 递归加载所有 JSON 文件
    loadQuestionsRecursive(bankPath, questions);
    
    return questions;
}

void PracticeWidget::loadQuestionsRecursive(const QString &dirPath, QVector<Question> &questions) const
{
    QDir dir(dirPath);
    QStringList filters;
    filters << "*.md" << "*.json";  // 优先MD，兼容JSON
    
    // 加载当前目录的题目文件
    QFileInfoList files = dir.entryInfoList(filters, QDir::Files);
    
    // 去重：如果同名的MD和JSON都存在，只加载MD
    QSet<QString> loadedFiles;
    
    for (const auto &fileInfo : files) {
        QString filePath = fileInfo.absoluteFilePath();
        QString fileName = fileInfo.fileName();
        QString baseName = fileInfo.completeBaseName();
        
        // 过滤配置文件和规律文件（使用精确匹配）
        if (fileName.endsWith("_parse_rule.json", Qt::CaseInsensitive) ||
            fileName == "出题模式规律.md" ||
            fileName == "出题模式规律.json" ||
            fileName.endsWith("_规律.md") ||
            fileName.endsWith("_pattern.md") ||
            fileName.startsWith(".")) {
            continue;
        }
        
        QString lowerName = fileName.toLower();
        if (lowerName == "readme.md" || 
            lowerName == "readme.txt" ||
            lowerName == "拆分规则.md" ||
            lowerName == "config.json" || 
            lowerName == "settings.json") {
            continue;
        }
        
        // 如果已经加载过这个文件名，跳过
        if (loadedFiles.contains(baseName)) {
            continue;
        }
        
        if (filePath.endsWith(".md", Qt::CaseInsensitive)) {
            // 加载MD文件
            Question q = Question::fromMarkdownFile(filePath);
            if (!q.id().isEmpty()) {
                questions.append(q);
            }
            loadedFiles.insert(baseName);
        } else if (filePath.endsWith(".json", Qt::CaseInsensitive)) {
            // 加载JSON文件
            QFile file(filePath);
            if (file.open(QIODevice::ReadOnly)) {
                QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
                
                if (doc.isArray()) {
                    QJsonArray arr = doc.array();
                    for (const auto &val : arr) {
                        questions.append(Question(val.toObject()));
                    }
                } else if (doc.isObject()) {
                    questions.append(Question(doc.object()));
                }
                
                file.close();
            }
            loadedFiles.insert(baseName);
        }
    }
    
    // 递归扫描子目录
    QFileInfoList subDirs = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const auto &subDirInfo : subDirs) {
        loadQuestionsRecursive(subDirInfo.absoluteFilePath(), questions);
    }
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
            return "✅";  // AI判题通过和运行测试通过都显示绿色对钩
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

void PracticeWidget::onQuestionStatusUpdated(const QString &questionId)
{
    // 查找对应行
    for (int row = 0; row < m_questionTable->rowCount(); ++row) {
        QTableWidgetItem *idItem = m_questionTable->item(row, 1);
        if (idItem && idItem->data(Qt::UserRole).toString() == questionId) {
            // 更新状态图标
            QString icon = getStatusIcon(questionId);
            QTableWidgetItem *statusItem = m_questionTable->item(row, 0);
            if (statusItem) {
                statusItem->setText(icon);
                statusItem->setTextAlignment(Qt::AlignCenter);
            }
            
            // 更新正确率和尝试次数
            QuestionProgressRecord progress = ProgressManager::instance().getProgress(questionId);
            
            QString accuracyText = progress.attemptCount > 0 
                ? QString("%1%").arg(progress.accuracy(), 0, 'f', 1)
                : "-";
            QTableWidgetItem *accuracyItem = m_questionTable->item(row, 5);
            if (accuracyItem) {
                accuracyItem->setText(accuracyText);
            }
            
            QString attemptText = progress.attemptCount > 0 
                ? QString::number(progress.attemptCount)
                : "-";
            QTableWidgetItem *attemptItem = m_questionTable->item(row, 6);
            if (attemptItem) {
                attemptItem->setText(attemptText);
            }
            
            break;
        }
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
    
    qDebug() << "[PracticeWidget] Question double clicked, ID:" << questionId;
    
    // 从当前题库中查找题目
    QString currentBankId = QuestionBankManager::instance().getCurrentBankId();
    if (currentBankId.isEmpty()) {
        qWarning() << "[PracticeWidget] No current bank selected";
        return;
    }
    
    QuestionBankInfo bankInfo = QuestionBankManager::instance().getBankInfo(currentBankId);
    QVector<Question> allQuestions = loadQuestionsFromBank(bankInfo.path);
    
    qDebug() << "[PracticeWidget] Searching in" << allQuestions.size() << "questions";
    
    for (const Question &q : allQuestions) {
        if (q.id() == questionId) {
            qDebug() << "[PracticeWidget] Found question:" << q.title();
            qDebug() << "[PracticeWidget] Question description length:" << q.description().length();
            qDebug() << "[PracticeWidget] Question has" << q.testCases().size() << "test cases";
            emit questionSelected(q);
            break;
        }
    }
}

void PracticeWidget::onRefreshClicked()
{
    // 发出信号请求 MainWindow 重新加载题库
    emit reloadQuestionBankRequested();
    
    // 然后刷新显示
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
    qDebug() << "[PracticeWidget] refreshQuestionList() called";
    
    updateBankSelector();
    loadQuestions();
    updateStatistics();
}

void PracticeWidget::onRandomQuestionClicked()
{
    Question question = getRandomQuestion();
    if (!question.id().isEmpty()) {
        emit questionSelected(question);
    } else {
        QMessageBox::information(this, "提示", "没有可用的题目");
    }
}

void PracticeWidget::onRecommendQuestionClicked()
{
    Question question = getRecommendedQuestion();
    if (!question.id().isEmpty()) {
        emit questionSelected(question);
    } else {
        QMessageBox::information(this, "提示", "没有推荐的题目");
    }
}

void PracticeWidget::onSwitchBankClicked()
{
    emit switchBankRequested();
}

void PracticeWidget::onExportProgressClicked()
{
    exportProgressReport();
}

void PracticeWidget::onBatchMarkClicked()
{
    batchMarkStatus();
}

void PracticeWidget::onHeaderClicked(int logicalIndex)
{
    // 切换排序顺序
    if (m_sortColumn == logicalIndex) {
        m_sortOrder = (m_sortOrder == Qt::AscendingOrder) ? Qt::DescendingOrder : Qt::AscendingOrder;
    } else {
        m_sortColumn = logicalIndex;
        m_sortOrder = Qt::AscendingOrder;
    }
    
    // 根据列排序
    if (logicalIndex >= 0 && logicalIndex < m_questionTable->columnCount()) {
        m_questionTable->sortItems(logicalIndex, m_sortOrder);
    }
}

Question PracticeWidget::getRandomQuestion() const
{
    QString currentBankId = QuestionBankManager::instance().getCurrentBankId();
    if (currentBankId.isEmpty()) {
        return Question();
    }
    
    QuestionBankInfo bankInfo = QuestionBankManager::instance().getBankInfo(currentBankId);
    QVector<Question> allQuestions = loadQuestionsFromBank(bankInfo.path);
    
    if (allQuestions.isEmpty()) {
        return Question();
    }
    
    int randomIndex = QRandomGenerator::global()->bounded(allQuestions.size());
    return allQuestions[randomIndex];
}

Question PracticeWidget::getRecommendedQuestion() const
{
    QString currentBankId = QuestionBankManager::instance().getCurrentBankId();
    if (currentBankId.isEmpty()) {
        return Question();
    }
    
    QuestionBankInfo bankInfo = QuestionBankManager::instance().getBankInfo(currentBankId);
    QVector<Question> allQuestions = loadQuestionsFromBank(bankInfo.path);
    
    if (allQuestions.isEmpty()) {
        return Question();
    }
    
    // 推荐策略：优先推荐未开始或进行中的题目，按难度从易到难
    QVector<Question> notStarted;
    QVector<Question> inProgress;
    
    for (const Question &q : allQuestions) {
        QuestionProgressRecord progress = ProgressManager::instance().getProgress(q.id());
        if (progress.status == QuestionStatus::NotStarted) {
            notStarted.append(q);
        } else if (progress.status == QuestionStatus::InProgress) {
            inProgress.append(q);
        }
    }
    
    // 优先推荐进行中的题目
    if (!inProgress.isEmpty()) {
        return inProgress.first();
    }
    
    // 其次推荐未开始的简单题目
    if (!notStarted.isEmpty()) {
        // 按难度排序
        std::sort(notStarted.begin(), notStarted.end(), [](const Question &a, const Question &b) {
            return static_cast<int>(a.difficulty()) < static_cast<int>(b.difficulty());
        });
        return notStarted.first();
    }
    
    // 都完成了，返回第一题
    return allQuestions.first();
}

void PracticeWidget::exportProgressReport()
{
    QString currentBankId = QuestionBankManager::instance().getCurrentBankId();
    if (currentBankId.isEmpty()) {
        QMessageBox::warning(this, "提示", "未选择题库，无法导出报告");
        return;
    }
    
    QuestionBankInfo bankInfo = QuestionBankManager::instance().getBankInfo(currentBankId);
    QVector<Question> allQuestions = loadQuestionsFromBank(bankInfo.path);
    
    if (allQuestions.isEmpty()) {
        QMessageBox::warning(this, "提示", "题库为空，无法导出报告");
        return;
    }
    
    QString fileName = QFileDialog::getSaveFileName(
        this,
        "导出进度报告",
        QString("刷题进度报告_%1.txt").arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss")),
        "文本文件 (*.txt);;所有文件 (*.*)"
    );
    
    if (fileName.isEmpty()) {
        return;
    }
    
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "错误", "无法创建文件");
        return;
    }
    
    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    
    // 写入报告头
    out << "========================================\n";
    out << "         刷题进度报告\n";
    out << "========================================\n";
    out << "生成时间: " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "\n";
    out << "题库名称: " << m_bankSelector->currentText() << "\n";
    out << "\n";
    
    // 统计信息
    int total = allQuestions.size();
    int completed = ProgressManager::instance().getCompletedCount();
    int mastered = ProgressManager::instance().getMasteredCount();
    double accuracy = ProgressManager::instance().getOverallAccuracy();
    
    out << "========================================\n";
    out << "         总体统计\n";
    out << "========================================\n";
    out << "总题数: " << total << "\n";
    out << "已完成: " << completed << " (" << (total > 0 ? completed * 100 / total : 0) << "%)\n";
    out << "已掌握: " << mastered << " (" << (total > 0 ? mastered * 100 / total : 0) << "%)\n";
    out << "总正确率: " << QString::number(accuracy, 'f', 1) << "%\n";
    out << "\n";
    
    // 详细题目列表
    out << "========================================\n";
    out << "         题目详情\n";
    out << "========================================\n";
    out << QString("%-6s %-40s %-8s %-10s %-10s %-10s\n")
        .arg("题号").arg("题目").arg("难度").arg("状态").arg("正确率").arg("尝试次数");
    out << "----------------------------------------\n";
    
    for (int i = 0; i < allQuestions.size(); ++i) {
        const Question &q = allQuestions[i];
        QuestionProgressRecord progress = ProgressManager::instance().getProgress(q.id());
        
        QString diffText;
        switch (q.difficulty()) {
            case Difficulty::Easy: diffText = "简单"; break;
            case Difficulty::Medium: diffText = "中等"; break;
            case Difficulty::Hard: diffText = "困难"; break;
        }
        
        QString statusText = getStatusText(q.id());
        QString accuracyText = progress.attemptCount > 0 
            ? QString("%1%").arg(progress.accuracy(), 0, 'f', 1)
            : "-";
        QString attemptText = progress.attemptCount > 0 
            ? QString::number(progress.attemptCount)
            : "-";
        
        out << QString("%-6d %-40s %-8s %-10s %-10s %-10s\n")
            .arg(i + 1)
            .arg(q.title().left(38))
            .arg(diffText)
            .arg(statusText)
            .arg(accuracyText)
            .arg(attemptText);
    }
    
    file.close();
    
    QMessageBox::information(this, "导出成功",
        QString("进度报告已导出到：\n%1").arg(fileName));
}

void PracticeWidget::batchMarkStatus()
{
    QList<QTableWidgetItem*> selectedItems = m_questionTable->selectedItems();
    if (selectedItems.isEmpty()) {
        QMessageBox::information(this, "提示", "请先选择要标记的题目");
        return;
    }
    
    // 获取选中的行（去重）
    QSet<int> selectedRows;
    for (QTableWidgetItem *item : selectedItems) {
        selectedRows.insert(item->row());
    }
    
    // 询问要标记的状态
    QStringList statusOptions;
    statusOptions << "未开始" << "进行中" << "已完成" << "已掌握";
    
    bool ok;
    QString selectedStatus = QInputDialog::getItem(
        this,
        "批量标记",
        QString("为选中的 %1 道题目标记状态：").arg(selectedRows.size()),
        statusOptions,
        0,
        false,
        &ok
    );
    
    if (!ok) {
        return;
    }
    
    QuestionStatus newStatus;
    if (selectedStatus == "未开始") {
        newStatus = QuestionStatus::NotStarted;
    } else if (selectedStatus == "进行中") {
        newStatus = QuestionStatus::InProgress;
    } else if (selectedStatus == "已完成") {
        newStatus = QuestionStatus::Completed;
    } else {
        newStatus = QuestionStatus::Mastered;
    }
    
    // 批量更新状态
    for (int row : selectedRows) {
        QTableWidgetItem *item = m_questionTable->item(row, 1);
        if (item) {
            QString questionId = item->data(Qt::UserRole).toString();
            ProgressManager::instance().updateStatus(questionId, newStatus);
        }
    }
    
    // 刷新显示
    loadQuestions();
    updateStatistics();
    
    QMessageBox::information(this, "完成",
        QString("已为 %1 道题目标记为【%2】").arg(selectedRows.size()).arg(selectedStatus));
}

bool PracticeWidget::eventFilter(QObject *watched, QEvent *event)
{
    // 问题1：拦截筛选器的滚轮事件，防止改变选项
    if (event->type() == QEvent::Wheel) {
        if (watched == m_difficultyFilter || watched == m_tagFilter || 
            watched == m_statusFilter || watched == m_sortCombo) {
            // 吞掉滚轮事件，不让它改变下拉框选项
            return true;
        }
        
        // 问题2：拦截表格的滚轮事件，防止传递给父容器
        if (watched == m_questionTable || watched == m_questionTable->viewport()) {
            QWheelEvent *wheelEvent = static_cast<QWheelEvent*>(event);
            QScrollBar *vScrollBar = m_questionTable->verticalScrollBar();
            
            if (vScrollBar) {
                int delta = wheelEvent->angleDelta().y();
                
                // 向上滚动且已经在顶部，或向下滚动且已经在底部
                bool atTop = (vScrollBar->value() == vScrollBar->minimum());
                bool atBottom = (vScrollBar->value() == vScrollBar->maximum());
                
                if ((delta > 0 && atTop) || (delta < 0 && atBottom)) {
                    // 在边界时，吞掉事件，不传递给父容器
                    return true;
                }
                
                // 不在边界时，让表格自己处理滚动
                return false;
            }
            
            // 没有滚动条时，吞掉事件
            return true;
        }
    }
    
    return QWidget::eventFilter(watched, event);
}
