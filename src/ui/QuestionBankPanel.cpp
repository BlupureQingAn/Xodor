#include "QuestionBankPanel.h"
#include "QuestionBankTreeWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QCheckBox>

QuestionBankPanel::QuestionBankPanel(QWidget *parent)
    : QWidget(parent)
    , m_searchEdit(nullptr)
    , m_treeWidget(nullptr)
    , m_easyCheckBox(nullptr)
    , m_mediumCheckBox(nullptr)
    , m_hardCheckBox(nullptr)
{
    // 默认显示所有难度
    m_activeDifficultyFilters.insert(Difficulty::Easy);
    m_activeDifficultyFilters.insert(Difficulty::Medium);
    m_activeDifficultyFilters.insert(Difficulty::Hard);
    
    setupUI();
}

void QuestionBankPanel::setupUI()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(5, 5, 5, 5);
    layout->setSpacing(8);
    
    // 搜索框
    QLabel *searchLabel = new QLabel("搜索:", this);
    searchLabel->setStyleSheet("color: #e8e8e8;");
    
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText("搜索题库或题目...");
    m_searchEdit->setStyleSheet(R"(
        QLineEdit {
            background-color: #2d2d2d;
            color: #e8e8e8;
            border: 1px solid #3a3a3a;
            border-radius: 4px;
            padding: 6px;
        }
        QLineEdit:focus {
            border: 1px solid #660000;
        }
    )");
    
    // 难度筛选
    QLabel *filterLabel = new QLabel("难度筛选:", this);
    filterLabel->setStyleSheet("color: #e8e8e8; font-weight: bold;");
    
    QHBoxLayout *filterLayout = new QHBoxLayout();
    filterLayout->setSpacing(10);
    
    m_easyCheckBox = new QCheckBox("简单", this);
    m_easyCheckBox->setChecked(true);
    m_easyCheckBox->setStyleSheet(R"(
        QCheckBox {
            color: #4CAF50;
            spacing: 5px;
        }
        QCheckBox::indicator {
            width: 16px;
            height: 16px;
            border: 1px solid #3a3a3a;
            border-radius: 3px;
            background-color: #2d2d2d;
        }
        QCheckBox::indicator:checked {
            background-color: #4CAF50;
            border-color: #4CAF50;
        }
        QCheckBox::indicator:hover {
            border-color: #4CAF50;
        }
    )");
    
    m_mediumCheckBox = new QCheckBox("中等", this);
    m_mediumCheckBox->setChecked(true);
    m_mediumCheckBox->setStyleSheet(R"(
        QCheckBox {
            color: #FFA500;
            spacing: 5px;
        }
        QCheckBox::indicator {
            width: 16px;
            height: 16px;
            border: 1px solid #3a3a3a;
            border-radius: 3px;
            background-color: #2d2d2d;
        }
        QCheckBox::indicator:checked {
            background-color: #FFA500;
            border-color: #FFA500;
        }
        QCheckBox::indicator:hover {
            border-color: #FFA500;
        }
    )");
    
    m_hardCheckBox = new QCheckBox("困难", this);
    m_hardCheckBox->setChecked(true);
    m_hardCheckBox->setStyleSheet(R"(
        QCheckBox {
            color: #F44336;
            spacing: 5px;
        }
        QCheckBox::indicator {
            width: 16px;
            height: 16px;
            border: 1px solid #3a3a3a;
            border-radius: 3px;
            background-color: #2d2d2d;
        }
        QCheckBox::indicator:checked {
            background-color: #F44336;
            border-color: #F44336;
        }
        QCheckBox::indicator:hover {
            border-color: #F44336;
        }
    )");
    
    filterLayout->addWidget(m_easyCheckBox);
    filterLayout->addWidget(m_mediumCheckBox);
    filterLayout->addWidget(m_hardCheckBox);
    filterLayout->addStretch();
    
    // 题库树
    m_treeWidget = new QuestionBankTreeWidget(this);
    
    // 布局
    layout->addWidget(searchLabel);
    layout->addWidget(m_searchEdit);
    layout->addWidget(filterLabel);
    layout->addLayout(filterLayout);
    layout->addWidget(m_treeWidget);
    
    // 连接信号
    connect(m_searchEdit, &QLineEdit::textChanged,
            this, &QuestionBankPanel::onSearchTextChanged);
    connect(m_treeWidget, &QuestionBankTreeWidget::questionSelected,
            this, &QuestionBankPanel::onQuestionSelected);
    connect(m_treeWidget, &QuestionBankTreeWidget::bankSelected,
            this, &QuestionBankPanel::onBankSelected);
    
    connect(m_easyCheckBox, &QCheckBox::stateChanged,
            this, &QuestionBankPanel::onDifficultyFilterChanged);
    connect(m_mediumCheckBox, &QCheckBox::stateChanged,
            this, &QuestionBankPanel::onDifficultyFilterChanged);
    connect(m_hardCheckBox, &QCheckBox::stateChanged,
            this, &QuestionBankPanel::onDifficultyFilterChanged);
}

void QuestionBankPanel::refreshBankTree()
{
    if (m_treeWidget) {
        m_treeWidget->refreshTree();
    }
}

void QuestionBankPanel::expandBank(const QString &bankPath)
{
    if (m_treeWidget) {
        m_treeWidget->expandBank(bankPath);
    }
}

void QuestionBankPanel::selectQuestion(const QString &questionPath)
{
    if (m_treeWidget) {
        m_treeWidget->selectQuestion(questionPath);
    }
}

void QuestionBankPanel::updateQuestionStatus(const QString &questionId)
{
    if (m_treeWidget) {
        m_treeWidget->updateQuestionStatus(questionId);
    }
}

void QuestionBankPanel::onSearchTextChanged(const QString &text)
{
    // TODO: 实现搜索过滤功能
    Q_UNUSED(text);
}

void QuestionBankPanel::onQuestionSelected(const QString &filePath, const Question &question)
{
    emit questionFileSelected(filePath, question);
}

void QuestionBankPanel::onBankSelected(const QString &bankPath)
{
    emit bankSelected(bankPath);
}

void QuestionBankPanel::onDifficultyFilterChanged()
{
    // 更新激活的难度筛选
    m_activeDifficultyFilters.clear();
    
    if (m_easyCheckBox->isChecked()) {
        m_activeDifficultyFilters.insert(Difficulty::Easy);
    }
    if (m_mediumCheckBox->isChecked()) {
        m_activeDifficultyFilters.insert(Difficulty::Medium);
    }
    if (m_hardCheckBox->isChecked()) {
        m_activeDifficultyFilters.insert(Difficulty::Hard);
    }
    
    // 应用筛选
    applyFilters();
    
    qDebug() << "[QuestionBankPanel] Difficulty filters changed. Active:" 
             << m_activeDifficultyFilters.size() << "filters";
}

void QuestionBankPanel::applyFilters()
{
    if (m_treeWidget) {
        m_treeWidget->setDifficultyFilter(m_activeDifficultyFilters);
    }
}

QSet<Difficulty> QuestionBankPanel::getActiveDifficultyFilters() const
{
    return m_activeDifficultyFilters;
}

void QuestionBankPanel::restoreDifficultyFilters(const QSet<Difficulty> &filters)
{
    m_activeDifficultyFilters = filters;
    
    // 更新复选框状态（不触发信号）
    m_easyCheckBox->blockSignals(true);
    m_mediumCheckBox->blockSignals(true);
    m_hardCheckBox->blockSignals(true);
    
    m_easyCheckBox->setChecked(filters.contains(Difficulty::Easy));
    m_mediumCheckBox->setChecked(filters.contains(Difficulty::Medium));
    m_hardCheckBox->setChecked(filters.contains(Difficulty::Hard));
    
    m_easyCheckBox->blockSignals(false);
    m_mediumCheckBox->blockSignals(false);
    m_hardCheckBox->blockSignals(false);
    
    // 应用筛选
    applyFilters();
    
    qDebug() << "[QuestionBankPanel] Restored difficulty filters:" << filters.size();
}

QStringList QuestionBankPanel::getExpandedPaths() const
{
    if (m_treeWidget) {
        return m_treeWidget->getExpandedPaths();
    }
    return QStringList();
}

void QuestionBankPanel::restoreExpandedPaths(const QStringList &paths)
{
    if (m_treeWidget) {
        m_treeWidget->restoreExpandedPaths(paths);
    }
}

QString QuestionBankPanel::getSelectedQuestionPath() const
{
    if (m_treeWidget) {
        return m_treeWidget->getSelectedQuestionPath();
    }
    return QString();
}
