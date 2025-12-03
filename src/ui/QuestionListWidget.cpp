#include "QuestionListWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QKeyEvent>
#include <QContextMenuEvent>
#include <QMenu>
#include <QMessageBox>
#include <QInputDialog>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonArray>

QuestionListWidget::QuestionListWidget(QWidget *parent)
    : QWidget(parent)
    , m_mode(ListMode::Questions)
{
    setupUI();
}

void QuestionListWidget::setupUI()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(5, 5, 5, 5);
    
    // 搜索框
    QLabel *searchLabel = new QLabel("搜索:", this);
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText("输入关键词...");
    
    // 难度筛选
    QLabel *filterLabel = new QLabel("难度筛选:", this);
    m_difficultyFilter = new QComboBox(this);
    m_difficultyFilter->addItem("全部", -1);
    m_difficultyFilter->addItem("简单", static_cast<int>(Difficulty::Easy));
    m_difficultyFilter->addItem("中等", static_cast<int>(Difficulty::Medium));
    m_difficultyFilter->addItem("困难", static_cast<int>(Difficulty::Hard));
    
    // 题目/题库列表
    m_questionList = new QListWidget(this);
    m_questionList->setAlternatingRowColors(true);
    m_questionList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_questionList->setContextMenuPolicy(Qt::DefaultContextMenu);
    
    // 题库管理按钮（仅在题库模式显示）
    QHBoxLayout *bankBtnLayout = new QHBoxLayout();
    m_loadBankBtn = new QPushButton("✓ 加载题库", this);
    m_deleteBankBtn = new QPushButton("🗑️ 删除", this);
    m_renameBankBtn = new QPushButton("✏️ 重命名", this);
    
    bankBtnLayout->addWidget(m_loadBankBtn);
    bankBtnLayout->addWidget(m_deleteBankBtn);
    bankBtnLayout->addWidget(m_renameBankBtn);
    
    // 默认隐藏题库按钮
    m_loadBankBtn->setVisible(false);
    m_deleteBankBtn->setVisible(false);
    m_renameBankBtn->setVisible(false);
    
    layout->addWidget(searchLabel);
    layout->addWidget(m_searchEdit);
    layout->addWidget(filterLabel);
    layout->addWidget(m_difficultyFilter);
    layout->addWidget(m_questionList);
    layout->addLayout(bankBtnLayout);
    
    // 连接信号
    connect(m_searchEdit, &QLineEdit::textChanged,
            this, &QuestionListWidget::onSearchTextChanged);
    connect(m_difficultyFilter, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &QuestionListWidget::onDifficultyFilterChanged);
    connect(m_questionList, &QListWidget::itemClicked,
            this, &QuestionListWidget::onQuestionItemClicked);
    connect(m_loadBankBtn, &QPushButton::clicked,
            this, &QuestionListWidget::onLoadBank);
    connect(m_deleteBankBtn, &QPushButton::clicked,
            this, &QuestionListWidget::onDeleteBank);
    connect(m_renameBankBtn, &QPushButton::clicked,
            this, &QuestionListWidget::onRenameBank);
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
    
    if (m_mode == ListMode::Questions) {
        // 题目列表模式的右键菜单
        QAction *selectAllAction = menu.addAction("🔘 全选 (Ctrl+A)");
        connect(selectAllAction, &QAction::triggered, this, &QuestionListWidget::onSelectAll);
        
        menu.addSeparator();
        
        QAction *deleteAction = menu.addAction("🗑️ 删除选中 (Delete)");
        deleteAction->setEnabled(!m_questionList->selectedItems().isEmpty());
        connect(deleteAction, &QAction::triggered, this, &QuestionListWidget::onDeleteSelected);
    } else {
        // 题库列表模式的右键菜单
        bool hasSelection = !m_questionList->selectedItems().isEmpty();
        
        QAction *loadAction = menu.addAction("✓ 加载题库");
        loadAction->setEnabled(hasSelection);
        connect(loadAction, &QAction::triggered, this, &QuestionListWidget::onLoadBank);
        
        menu.addSeparator();
        
        QAction *renameAction = menu.addAction("✏️ 重命名");
        renameAction->setEnabled(hasSelection);
        connect(renameAction, &QAction::triggered, this, &QuestionListWidget::onRenameBank);
        
        QAction *deleteAction = menu.addAction("🗑️ 删除");
        deleteAction->setEnabled(hasSelection);
        connect(deleteAction, &QAction::triggered, this, &QuestionListWidget::onDeleteBank);
    }
    
    menu.exec(event->globalPos());
}

void QuestionListWidget::setMode(ListMode mode)
{
    m_mode = mode;
    
    // 根据模式显示/隐藏控件
    if (m_mode == ListMode::QuestionBanks) {
        m_difficultyFilter->setVisible(false);
        m_loadBankBtn->setVisible(true);
        m_deleteBankBtn->setVisible(true);
        m_renameBankBtn->setVisible(true);
        m_searchEdit->setPlaceholderText("搜索题库...");
        refreshBankList();
    } else {
        m_difficultyFilter->setVisible(true);
        m_loadBankBtn->setVisible(false);
        m_deleteBankBtn->setVisible(false);
        m_renameBankBtn->setVisible(false);
        m_searchEdit->setPlaceholderText("输入题目标题...");
    }
}

void QuestionListWidget::refreshBankList()
{
    if (m_mode != ListMode::QuestionBanks) {
        return;
    }
    
    updateBankList();
}

void QuestionListWidget::updateBankList()
{
    m_questionList->clear();
    
    // 扫描基础题库目录
    QDir baseDir("data/基础题库");
    if (!baseDir.exists()) {
        return;
    }
    
    QStringList banks = baseDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    QString searchText = m_searchEdit->text().toLower();
    
    for (const QString &bankName : banks) {
        // 搜索过滤
        if (!searchText.isEmpty() && !bankName.toLower().contains(searchText)) {
            continue;
        }
        
        QString bankPath = baseDir.filePath(bankName);
        QString info = getBankInfo(bankPath);
        
        QListWidgetItem *item = new QListWidgetItem(info);
        item->setData(Qt::UserRole, bankPath);
        m_questionList->addItem(item);
    }
}

QString QuestionListWidget::getBankInfo(const QString &bankPath) const
{
    QFileInfo pathInfo(bankPath);
    QString bankName = pathInfo.fileName();
    
    // 读取questions.json获取题目数量
    QString jsonPath = bankPath + "/questions.json";
    int questionCount = 0;
    
    QFile jsonFile(jsonPath);
    if (jsonFile.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(jsonFile.readAll());
        jsonFile.close();
        
        if (doc.isArray()) {
            questionCount = doc.array().size();
        }
    }
    
    return QString("📚 %1 (%2 道题目)").arg(bankName).arg(questionCount);
}

void QuestionListWidget::onLoadBank()
{
    QListWidgetItem *item = m_questionList->currentItem();
    if (!item) {
        return;
    }
    
    m_selectedBankPath = item->data(Qt::UserRole).toString();
    emit bankLoadRequested(m_selectedBankPath);
}

void QuestionListWidget::onDeleteBank()
{
    QListWidgetItem *item = m_questionList->currentItem();
    if (!item) {
        return;
    }
    
    QString bankPath = item->data(Qt::UserRole).toString();
    QFileInfo pathInfo(bankPath);
    QString bankName = pathInfo.fileName();
    
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "确认删除",
        QString("确定要删除题库【%1】吗？\n\n此操作将删除：\n"
                "• 基础题库文件\n"
                "• 原始题库备份\n\n"
                "此操作不可恢复！").arg(bankName),
        QMessageBox::Yes | QMessageBox::No
    );
    
    if (reply == QMessageBox::Yes) {
        // 删除基础题库
        QDir baseDir(bankPath);
        if (baseDir.removeRecursively()) {
            // 删除原始题库
            QString originalPath = QString("data/原始题库/%1").arg(bankName);
            QDir originalDir(originalPath);
            originalDir.removeRecursively();
            
            QMessageBox::information(this, "删除成功", 
                QString("题库【%1】已删除").arg(bankName));
            
            refreshBankList();
        } else {
            QMessageBox::warning(this, "删除失败", 
                QString("无法删除题库【%1】").arg(bankName));
        }
    }
}

void QuestionListWidget::onRenameBank()
{
    QListWidgetItem *item = m_questionList->currentItem();
    if (!item) {
        return;
    }
    
    QString bankPath = item->data(Qt::UserRole).toString();
    QFileInfo pathInfo(bankPath);
    QString oldName = pathInfo.fileName();
    
    bool ok;
    QString newName = QInputDialog::getText(
        this,
        "重命名题库",
        "请输入新的题库名称：",
        QLineEdit::Normal,
        oldName,
        &ok
    );
    
    if (ok && !newName.isEmpty() && newName != oldName) {
        QString newPath = pathInfo.dir().filePath(newName);
        
        // 检查新名称是否已存在
        if (QDir(newPath).exists()) {
            QMessageBox::warning(this, "重命名失败", 
                QString("题库【%1】已存在").arg(newName));
            return;
        }
        
        // 重命名基础题库
        if (QDir().rename(bankPath, newPath)) {
            // 重命名原始题库
            QString oldOriginalPath = QString("data/原始题库/%1").arg(oldName);
            QString newOriginalPath = QString("data/原始题库/%1").arg(newName);
            QDir().rename(oldOriginalPath, newOriginalPath);
            
            QMessageBox::information(this, "重命名成功", 
                QString("题库已重命名为【%1】").arg(newName));
            
            refreshBankList();
        } else {
            QMessageBox::warning(this, "重命名失败", 
                "无法重命名题库");
        }
    }
}
