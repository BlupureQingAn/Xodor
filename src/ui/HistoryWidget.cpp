#include "HistoryWidget.h"
#include "../core/ProgressManager.h"
#include "../core/QuestionBankManager.h"
#include "../core/Question.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QMessageBox>

HistoryWidget::HistoryWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
    loadHistory();  // 自动加载历史记录
}

void HistoryWidget::setupUI()
{
    setWindowTitle("做题历史记录");
    resize(900, 600);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(15, 15, 15, 15);
    
    // 标题栏
    QHBoxLayout *titleLayout = new QHBoxLayout();
    QLabel *titleLabel = new QLabel("📊 做题历史记录", this);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(14);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    
    QPushButton *refreshButton = new QPushButton("🔄 刷新", this);
    connect(refreshButton, &QPushButton::clicked, this, &HistoryWidget::loadHistory);
    
    titleLayout->addWidget(titleLabel);
    titleLayout->addStretch();
    titleLayout->addWidget(refreshButton);
    
    mainLayout->addLayout(titleLayout);
    
    // 统计信息
    QHBoxLayout *statsLayout = new QHBoxLayout();
    
    m_totalLabel = new QLabel("总题数: 0", this);
    m_completedLabel = new QLabel("已完成: 0", this);
    m_accuracyLabel = new QLabel("正确率: 0%", this);
    
    statsLayout->addWidget(m_totalLabel);
    statsLayout->addWidget(m_completedLabel);
    statsLayout->addWidget(m_accuracyLabel);
    statsLayout->addStretch();
    
    mainLayout->addLayout(statsLayout);
    
    // 历史记录表格
    m_historyTable = new QTableWidget(this);
    m_historyTable->setColumnCount(6);
    m_historyTable->setHorizontalHeaderLabels(
        {"题目ID", "题目标题", "状态", "尝试次数", "正确率", "最后提交时间"});
    
    m_historyTable->horizontalHeader()->setStretchLastSection(true);
    m_historyTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_historyTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_historyTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_historyTable->setAlternatingRowColors(true);
    
    // 设置列宽
    m_historyTable->setColumnWidth(0, 150);
    m_historyTable->setColumnWidth(1, 250);
    m_historyTable->setColumnWidth(2, 100);
    m_historyTable->setColumnWidth(3, 80);
    m_historyTable->setColumnWidth(4, 80);
    
    mainLayout->addWidget(m_historyTable);
    
    // 样式
    setStyleSheet(R"(
        QWidget {
            background-color: #1e1e1e;
            color: #e8e8e8;
        }
        QTableWidget {
            background-color: #252525;
            gridline-color: #3a3a3a;
            border: 1px solid #3a3a3a;
        }
        QTableWidget::item {
            padding: 5px;
        }
        QTableWidget::item:selected {
            background-color: #660000;
        }
        QHeaderView::section {
            background-color: #2a2a2a;
            color: #e8e8e8;
            padding: 5px;
            border: 1px solid #3a3a3a;
            font-weight: bold;
        }
        QPushButton {
            background-color: #660000;
            color: white;
            border: none;
            padding: 5px 15px;
            border-radius: 4px;
        }
        QPushButton:hover {
            background-color: #880000;
        }
    )");
}

void HistoryWidget::loadHistory()
{
    m_historyTable->setRowCount(0);
    
    ProgressManager &pm = ProgressManager::instance();
    QuestionBankManager &qbm = QuestionBankManager::instance();
    
    // 获取所有有进度记录的题目
    QStringList allStatuses;
    allStatuses << pm.getQuestionsByStatus(QuestionStatus::NotStarted)
                << pm.getQuestionsByStatus(QuestionStatus::InProgress)
                << pm.getQuestionsByStatus(QuestionStatus::Completed)
                << pm.getQuestionsByStatus(QuestionStatus::Mastered);
    
    // 去重
    QSet<QString> uniqueQuestions(allStatuses.begin(), allStatuses.end());
    QList<QString> questionIds = uniqueQuestions.values();
    
    // 按最后提交时间排序（最新的在前）
    std::sort(questionIds.begin(), questionIds.end(), [&pm](const QString &a, const QString &b) {
        QuestionProgressRecord recA = pm.getProgress(a);
        QuestionProgressRecord recB = pm.getProgress(b);
        return recA.lastAttemptTime > recB.lastAttemptTime;
    });
    
    int totalAttempts = 0;
    int totalCorrect = 0;
    
    for (const QString &questionId : questionIds) {
        QuestionProgressRecord record = pm.getProgress(questionId);
        
        // 跳过没有尝试过的题目
        if (record.attemptCount == 0) {
            continue;
        }
        
        int row = m_historyTable->rowCount();
        m_historyTable->insertRow(row);
        
        // 题目ID
        m_historyTable->setItem(row, 0, new QTableWidgetItem(questionId));
        
        // 题目标题（从进度记录中获取）
        QString title = record.questionTitle;
        if (title.isEmpty()) {
            title = questionId;  // 如果没有标题，显示ID
        }
        m_historyTable->setItem(row, 1, new QTableWidgetItem(title));
        
        // 状态
        QString statusText;
        QString statusColor;
        switch (record.status) {
            case QuestionStatus::NotStarted:
                statusText = "未开始";
                statusColor = "#888";
                break;
            case QuestionStatus::InProgress:
                statusText = "进行中";
                statusColor = "#ffa500";
                break;
            case QuestionStatus::Completed:
                statusText = "已完成";
                statusColor = "#00ff00";
                break;
            case QuestionStatus::Mastered:
                statusText = "已掌握";
                statusColor = "#ffd700";
                break;
        }
        QTableWidgetItem *statusItem = new QTableWidgetItem(statusText);
        statusItem->setForeground(QColor(statusColor));
        m_historyTable->setItem(row, 2, statusItem);
        
        // 尝试次数
        m_historyTable->setItem(row, 3, new QTableWidgetItem(QString::number(record.attemptCount)));
        
        // 正确率
        double accuracy = record.attemptCount > 0 
            ? (record.correctCount * 100.0 / record.attemptCount) 
            : 0.0;
        m_historyTable->setItem(row, 4, new QTableWidgetItem(QString::number(accuracy, 'f', 1) + "%"));
        
        // 最后提交时间
        QString timeStr = record.lastAttemptTime.isValid() 
            ? record.lastAttemptTime.toString("yyyy-MM-dd hh:mm:ss")
            : "未知";
        m_historyTable->setItem(row, 5, new QTableWidgetItem(timeStr));
        
        totalAttempts += record.attemptCount;
        totalCorrect += record.correctCount;
    }
    
    // 更新统计信息
    m_totalLabel->setText(QString("总题数: %1").arg(m_historyTable->rowCount()));
    m_completedLabel->setText(QString("已完成: %1").arg(pm.getCompletedCount()));
    
    double overallAccuracy = totalAttempts > 0 
        ? (totalCorrect * 100.0 / totalAttempts) 
        : 0.0;
    m_accuracyLabel->setText(QString("正确率: %1%").arg(QString::number(overallAccuracy, 'f', 1)));
    
    if (m_historyTable->rowCount() == 0) {
        QMessageBox::information(this, "提示", "暂无做题历史记录");
    }
}
