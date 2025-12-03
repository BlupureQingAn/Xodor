#include "QuestionListWidget.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QKeyEvent>
#include <QContextMenuEvent>
#include <QMenu>

QuestionListWidget::QuestionListWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
}

void QuestionListWidget::setupUI()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(5, 5, 5, 5);
    
    // 搜索框
    QLabel *searchLabel = new QLabel("搜索题目:", this);
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText("输入题目标题...");
    
    // 难度筛选
    QLabel *filterLabel = new QLabel("难度筛选:", this);
    m_difficultyFilter = new QComboBox(this);
    m_difficultyFilter->addItem("全部", -1);
    m_difficultyFilter->addItem("简单", static_cast<int>(Difficulty::Easy));
    m_difficultyFilter->addItem("中等", static_cast<int>(Difficulty::Medium));
    m_difficultyFilter->addItem("困难", static_cast<int>(Difficulty::Hard));
    
    // 题目列表
    m_questionList = new QListWidget(this);
    m_questionList->setAlternatingRowColors(true);
    m_questionList->setSelectionMode(QAbstractItemView::ExtendedSelection);  // 支持多选
    m_questionList->setContextMenuPolicy(Qt::DefaultContextMenu);
    
    layout->addWidget(searchLabel);
    layout->addWidget(m_searchEdit);
    layout->addWidget(filterLabel);
    layout->addWidget(m_difficultyFilter);
    layout->addWidget(m_questionList);
    
    // 连接信号
    connect(m_searchEdit, &QLineEdit::textChanged,
            this, &QuestionListWidget::onSearchTextChanged);
    connect(m_difficultyFilter, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &QuestionListWidget::onDifficultyFilterChanged);
    connect(m_questionList, &QListWidget::itemClicked,
            this, &QuestionListWidget::onQuestionItemClicked);
}

void QuestionListWidget::setQuestions(const QVector<Question> &questions)
{
    m_allQuestions = questions;
    updateQuestionList();
}

void QuestionListWidget::setCurrentQuestion(int index)
{
    if (index >= 0 && index < m_questionList->count()) {
        m_questionList->setCurrentRow(index);
    }
}

void QuestionListWidget::onSearchTextChanged(const QString &text)
{
    Q_UNUSED(text);
    filterQuestions();
}

void QuestionListWidget::onDifficultyFilterChanged(int index)
{
    Q_UNUSED(index);
    filterQuestions();
}

void QuestionListWidget::onQuestionItemClicked(QListWidgetItem *item)
{
    int actualIndex = item->data(Qt::UserRole).toInt();
    emit questionSelected(actualIndex);
}

void QuestionListWidget::updateQuestionList()
{
    m_filteredIndices.clear();
    for (int i = 0; i < m_allQuestions.size(); ++i) {
        m_filteredIndices.append(i);
    }
    filterQuestions();
}

void QuestionListWidget::filterQuestions()
{
    m_questionList->clear();
    
    QString searchText = m_searchEdit->text().toLower();
    int difficultyFilter = m_difficultyFilter->currentData().toInt();
    
    for (int i = 0; i < m_allQuestions.size(); ++i) {
        const Question &q = m_allQuestions[i];
        
        // 难度筛选
        if (difficultyFilter >= 0 && static_cast<int>(q.difficulty()) != difficultyFilter) {
            continue;
        }
        
        // 搜索筛选
        if (!searchText.isEmpty() && !q.title().toLower().contains(searchText)) {
            continue;
        }
        
        // 添加到列表
        QString difficultyText;
        QString colorStyle;
        switch (q.difficulty()) {
            case Difficulty::Easy:
                difficultyText = "简单";
                colorStyle = "color: #e8e8e8;";
                break;
            case Difficulty::Medium:
                difficultyText = "中等";
                colorStyle = "color: #b0b0b0;";
                break;
            case Difficulty::Hard:
                difficultyText = "困难";
                colorStyle = "color: #660000;";
                break;
        }
        
        QString itemText = QString("%1. %2 [%3]")
            .arg(i + 1)
            .arg(q.title())
            .arg(difficultyText);
        
        QListWidgetItem *item = new QListWidgetItem(itemText);
        item->setData(Qt::UserRole, i);
        m_questionList->addItem(item);
    }
}

QVector<int> QuestionListWidget::getSelectedIndices() const
{
    QVector<int> indices;
    QList<QListWidgetItem*> selectedItems = m_questionList->selectedItems();
    
    for (QListWidgetItem *item : selectedItems) {
        int row = m_questionList->row(item);
        if (row >= 0 && row < m_filteredIndices.size()) {
            indices.append(m_filteredIndices[row]);
        }
    }
    
    return indices;
}

void QuestionListWidget::onDeleteSelected()
{
    QVector<int> indices = getSelectedIndices();
    if (!indices.isEmpty()) {
        emit questionsDeleteRequested(indices);
    }
}

void QuestionListWidget::onSelectAll()
{
    m_questionList->selectAll();
}

void QuestionListWidget::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Delete) {
        // Delete键删除选中的题目
        onDeleteSelected();
        event->accept();
    } else if (event->matches(QKeySequence::SelectAll)) {
        // Ctrl+A全选
        onSelectAll();
        event->accept();
    } else {
        QWidget::keyPressEvent(event);
    }
}

void QuestionListWidget::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu(this);
    
    QAction *selectAllAction = menu.addAction("🔘 全选 (Ctrl+A)");
    connect(selectAllAction, &QAction::triggered, this, &QuestionListWidget::onSelectAll);
    
    menu.addSeparator();
    
    QAction *deleteAction = menu.addAction("🗑️ 删除选中 (Delete)");
    deleteAction->setEnabled(!m_questionList->selectedItems().isEmpty());
    connect(deleteAction, &QAction::triggered, this, &QuestionListWidget::onDeleteSelected);
    
    menu.exec(event->globalPos());
}
