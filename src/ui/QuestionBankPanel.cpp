#include "QuestionBankPanel.h"
#include "QuestionBankTreeWidget.h"
#include <QVBoxLayout>
#include <QLabel>

QuestionBankPanel::QuestionBankPanel(QWidget *parent)
    : QWidget(parent)
    , m_searchEdit(nullptr)
    , m_treeWidget(nullptr)
{
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
    
    // 题库树
    m_treeWidget = new QuestionBankTreeWidget(this);
    
    // 布局
    layout->addWidget(searchLabel);
    layout->addWidget(m_searchEdit);
    layout->addWidget(m_treeWidget);
    
    // 连接信号
    connect(m_searchEdit, &QLineEdit::textChanged,
            this, &QuestionBankPanel::onSearchTextChanged);
    connect(m_treeWidget, &QuestionBankTreeWidget::questionSelected,
            this, &QuestionBankPanel::onQuestionSelected);
    connect(m_treeWidget, &QuestionBankTreeWidget::bankSelected,
            this, &QuestionBankPanel::onBankSelected);
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
