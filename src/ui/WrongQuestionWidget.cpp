#include "WrongQuestionWidget.h"
#include "../core/WrongQuestionBook.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>

WrongQuestionWidget::WrongQuestionWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
    loadWrongQuestions();
}

void WrongQuestionWidget::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(12);
    
    // 标题和统计
    QHBoxLayout *headerLayout = new QHBoxLayout();
    QLabel *titleLabel = new QLabel("错题本", this);
    titleLabel->setStyleSheet("font-size: 16pt; font-weight: bold; color: #e8e8e8;");
    
    m_statsLabel = new QLabel(this);
    m_statsLabel->setStyleSheet("color: #b0b0b0;");
    
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    headerLayout->addWidget(m_statsLabel);
    
    // 筛选按钮
    QHBoxLayout *filterLayout = new QHBoxLayout();
    
    m_showAllBtn = new QPushButton("全部", this);
    m_showUnresolvedBtn = new QPushButton("未解决", this);
    m_clearBtn = new QPushButton("清空错题本", this);
    
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
    
    m_showAllBtn->setStyleSheet(btnStyle);
    m_showUnresolvedBtn->setStyleSheet(btnStyle);
    m_clearBtn->setStyleSheet(btnStyle);
    
    filterLayout->addWidget(m_showAllBtn);
    filterLayout->addWidget(m_showUnresolvedBtn);
    filterLayout->addStretch();
    filterLayout->addWidget(m_clearBtn);
    
    // 错题列表
    m_wrongQuestionsTable = new QTableWidget(this);
    m_wrongQuestionsTable->setColumnCount(6);
    m_wrongQuestionsTable->setHorizontalHeaderLabels({
        "题目", "难度", "错误次数", "最后尝试时间", "状态", "操作"
    });
    
    m_wrongQuestionsTable->horizontalHeader()->setStretchLastSection(false);
    m_wrongQuestionsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_wrongQuestionsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_wrongQuestionsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_wrongQuestionsTable->setAlternatingRowColors(true);
    
    m_wrongQuestionsTable->setStyleSheet(R"(
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
    mainLayout->addWidget(m_wrongQuestionsTable);
    
    setWindowTitle("错题本");
    resize(1000, 600);
    
    // 连接信号
    connect(m_showAllBtn, &QPushButton::clicked, this, &WrongQuestionWidget::showAllQuestions);
    connect(m_showUnresolvedBtn, &QPushButton::clicked, this, &WrongQuestionWidget::showUnresolvedQuestions);
    connect(m_clearBtn, &QPushButton::clicked, this, &WrongQuestionWidget::clearWrongQuestions);
    connect(m_wrongQuestionsTable, &QTableWidget::cellDoubleClicked, this, &WrongQuestionWidget::onQuestionDoubleClicked);
}

void WrongQuestionWidget::loadWrongQuestions()
{
    WrongQuestionBook::instance().load();
    showAllQuestions();
}

void WrongQuestionWidget::showAllQuestions()
{
    displayQuestions(WrongQuestionBook::instance().getAllWrongQuestions());
}

void WrongQuestionWidget::showUnresolvedQuestions()
{
    displayQuestions(WrongQuestionBook::instance().getUnresolvedQuestions());
}

void WrongQuestionWidget::displayQuestions(const QVector<WrongQuestionRecord> &questions)
{
    m_wrongQuestionsTable->setRowCount(0);
    
    int total = WrongQuestionBook::instance().getWrongQuestionCount();
    int unresolved = WrongQuestionBook::instance().getUnresolvedCount();
    m_statsLabel->setText(QString("总计: %1 题 | 未解决: %2 题").arg(total).arg(unresolved));
    
    for (const auto &record : questions) {
        int row = m_wrongQuestionsTable->rowCount();
        m_wrongQuestionsTable->insertRow(row);
        
        // 题目
        m_wrongQuestionsTable->setItem(row, 0, new QTableWidgetItem(record.questionTitle));
        
        // 难度
        QString diffText;
        QString diffColor;
        switch (record.difficulty) {
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
        m_wrongQuestionsTable->setItem(row, 1, diffItem);
        
        // 错误次数
        m_wrongQuestionsTable->setItem(row, 2, new QTableWidgetItem(QString::number(record.attemptCount)));
        
        // 时间
        m_wrongQuestionsTable->setItem(row, 3, new QTableWidgetItem(
            record.attemptTime.toString("yyyy-MM-dd hh:mm")));
        
        // 状态
        QString statusText = record.resolved ? "✓ 已解决" : "✗ 未解决";
        QString statusColor = record.resolved ? "#e8e8e8" : "#660000";
        QTableWidgetItem *statusItem = new QTableWidgetItem(statusText);
        statusItem->setForeground(QColor(statusColor));
        m_wrongQuestionsTable->setItem(row, 4, statusItem);
        
        // 操作按钮
        QPushButton *markBtn = new QPushButton(record.resolved ? "标记未解决" : "标记已解决", this);
        markBtn->setProperty("questionId", record.questionId);
        markBtn->setStyleSheet(R"(
            QPushButton {
                background-color: #660000;
                color: white;
                border: none;
                border-radius: 6px;
                padding: 6px 12px;
            }
            QPushButton:hover {
                background-color: #880000;
            }
        )");
        
        connect(markBtn, &QPushButton::clicked, this, &WrongQuestionWidget::onMarkButtonClicked);
        m_wrongQuestionsTable->setCellWidget(row, 5, markBtn);
    }
}

void WrongQuestionWidget::clearWrongQuestions()
{
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "确认清空",
        "确定要清空所有错题记录吗？此操作不可恢复。",
        QMessageBox::Yes | QMessageBox::No
    );
    
    if (reply == QMessageBox::Yes) {
        WrongQuestionBook::instance().clear();
        loadWrongQuestions();
    }
}

void WrongQuestionWidget::onMarkButtonClicked()
{
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;
    
    QString questionId = btn->property("questionId").toString();
    WrongQuestionBook::instance().markAsResolved(questionId);
    loadWrongQuestions();
}

void WrongQuestionWidget::onQuestionDoubleClicked(int row, int column)
{
    Q_UNUSED(column);
    QString questionTitle = m_wrongQuestionsTable->item(row, 0)->text();
    emit questionSelected(questionTitle);
}
