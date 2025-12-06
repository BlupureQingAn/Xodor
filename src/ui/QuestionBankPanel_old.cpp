#include "QuestionBankPanel.h"
#include "../utils/SessionManager.h"
#include "../core/ProgressManager.h"
#include "../core/QuestionBankManager.h"
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
#include <QJsonObject>

QuestionBankPanel::QuestionBankPanel(QWidget *parent)
    : QWidget(parent)
    , m_mode(PanelMode::Questions)
{
    setupUI();
}

void QuestionBankPanel::setupUI()
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
    m_questionList->setStyleSheet(R"(
        QListWidget {
            background-color: #2d2d2d;
            color: #e8e8e8;
            border: 1px solid #3a3a3a;
            border-radius: 4px;
            outline: none;
        }
        QListWidget::item {
            padding: 8px;
            border-bottom: 1px solid #3a3a3a;
            border: none;
            outline: none;
        }
        QListWidget::item:selected {
            background-color: #660000;
            color: #ffffff;
            border: none;
            outline: none;
        }
        QListWidget::item:selected:hover {
            background-color: #880000;
            outline: none;
        }
        QListWidget::item:hover {
            background-color: #323232;
        }
        QListWidget::item:focus {
            outline: none;
            border: none;
        }
    )");
    
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
            this, &QuestionBankPanel::onSearchTextChanged);
    connect(m_difficultyFilter, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &QuestionBankPanel::onDifficultyFilterChanged);
    connect(m_questionList, &QListWidget::itemClicked,
            this, &QuestionBankPanel::onQuestionItemClicked);
    connect(m_loadBankBtn, &QPushButton::clicked,
            this, &QuestionBankPanel::onLoadBank);
    connect(m_deleteBankBtn, &QPushButton::clicked,
            this, &QuestionBankPanel::onDeleteBank);
    connect(m_renameBankBtn, &QPushButton::clicked,
            this, &QuestionBankPanel::onRenameBank);
            
    // 连接进度更新信号
    connect(&ProgressManager::instance(), &ProgressManager::progressUpdated,
            this, &QuestionBankPanel::updateQuestionStatus);
}

void QuestionBankPanel::setQuestions(const QVector<Question> &questions)
{
    m_allQuestions = questions;
    updateQuestionList();
}

void QuestionBankPanel::setCurrentQuestion(int index)
{
    if (index >= 0 && index < m_questionList->count()) {
        m_questionList->setCurrentRow(index);
    }
}

void QuestionBankPanel::onSearchTextChanged(const QString &text)
{
    Q_UNUSED(text);
    filterQuestions();
}

void QuestionBankPanel::onDifficultyFilterChanged(int index)
{
    Q_UNUSED(index);
    filterQuestions();
}

void QuestionBankPanel::onQuestionItemClicked(QListWidgetItem *item)
{
    int actualIndex = item->data(Qt::UserRole).toInt();
    emit questionSelected(actualIndex);
}

void QuestionBankPanel::updateQuestionList()
{
    m_filteredIndices.clear();
    for (int i = 0; i < m_allQuestions.size(); ++i) {
        m_filteredIndices.append(i);
    }
    filterQuestions();
}

void QuestionBankPanel::filterQuestions()
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
        
        // 获取题目状态图标
        QString statusIcon = getQuestionStatusIcon(q.id());
        
        QString itemText = QString("%1 %2. %3 [%4]")
            .arg(statusIcon)
            .arg(i + 1)
            .arg(q.title())
            .arg(difficultyText);
        
        QListWidgetItem *item = new QListWidgetItem(itemText);
        item->setData(Qt::UserRole, i);
        m_questionList->addItem(item);
    }
}

QVector<int> QuestionBankPanel::getSelectedIndices() const
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

void QuestionBankPanel::onDeleteSelected()
{
    QVector<int> indices = getSelectedIndices();
    if (!indices.isEmpty()) {
        emit questionsDeleteRequested(indices);
    }
}

void QuestionBankPanel::onSelectAll()
{
    m_questionList->selectAll();
}

void QuestionBankPanel::keyPressEvent(QKeyEvent *event)
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

void QuestionBankPanel::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu(this);
    
    if (m_mode == PanelMode::Questions) {
        // 题目列表模式的右键菜单
        QAction *selectAllAction = menu.addAction("🔘 全选 (Ctrl+A)");
        connect(selectAllAction, &QAction::triggered, this, &QuestionBankPanel::onSelectAll);
        
        menu.addSeparator();
        
        QAction *deleteAction = menu.addAction("🗑️ 删除选中 (Delete)");
        deleteAction->setEnabled(!m_questionList->selectedItems().isEmpty());
        connect(deleteAction, &QAction::triggered, this, &QuestionBankPanel::onDeleteSelected);
    } else {
        // 题库列表模式的右键菜单
        bool hasSelection = !m_questionList->selectedItems().isEmpty();
        
        QAction *loadAction = menu.addAction("✓ 加载题库");
        loadAction->setEnabled(hasSelection);
        connect(loadAction, &QAction::triggered, this, &QuestionBankPanel::onLoadBank);
        
        menu.addSeparator();
        
        QAction *renameAction = menu.addAction("✏️ 重命名");
        renameAction->setEnabled(hasSelection);
        connect(renameAction, &QAction::triggered, this, &QuestionBankPanel::onRenameBank);
        
        QAction *deleteAction = menu.addAction("🗑️ 删除");
        deleteAction->setEnabled(hasSelection);
        connect(deleteAction, &QAction::triggered, this, &QuestionBankPanel::onDeleteBank);
    }
    
    menu.exec(event->globalPos());
}

void QuestionBankPanel::setMode(PanelMode mode)
{
    m_mode = mode;
    
    // 根据模式显示/隐藏控件
    if (m_mode == PanelMode::QuestionBanks) {
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

void QuestionBankPanel::refreshBankList()
{
    if (m_mode != PanelMode::QuestionBanks) {
        return;
    }
    
    updateBankList();
}

void QuestionBankPanel::updateBankList()
{
    m_questionList->clear();
    
    // 使用 QuestionBankManager 作为数据源
    QVector<QuestionBankInfo> banks = QuestionBankManager::instance().getAllBanks();
    QString searchText = m_searchEdit->text().toLower();
    QString currentBankId = QuestionBankManager::instance().getCurrentBankId();
    
    for (const QuestionBankInfo &info : banks) {
        // 搜索过滤
        if (!searchText.isEmpty() && !info.name.toLower().contains(searchText)) {
            continue;
        }
        
        // 实时统计题目数量
        int actualCount = countQuestionsInDirectory(info.path);
        
        QString displayText = QString("📚 %1 (%2 道题目)")
            .arg(info.name)
            .arg(actualCount);
        
        // 标记当前题库
        if (info.id == currentBankId) {
            displayText = "⭐ " + displayText;
        }
        
        QListWidgetItem *item = new QListWidgetItem(displayText);
        item->setData(Qt::UserRole, info.id);  // 存储题库ID而不是路径
        m_questionList->addItem(item);
    }
}

int QuestionBankPanel::countQuestionsInDirectory(const QString &dirPath) const
{
    int count = 0;
    QDir dir(dirPath);
    
    if (!dir.exists()) {
        return 0;
    }
    
    // 统计当前目录的 JSON 文件
    QStringList filters;
    filters << "*.json";
    QFileInfoList files = dir.entryInfoList(filters, QDir::Files);
    
    for (const auto &fileInfo : files) {
        QFile file(fileInfo.absoluteFilePath());
        if (file.open(QIODevice::ReadOnly)) {
            QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
            
            if (doc.isArray()) {
                count += doc.array().size();
            } else if (doc.isObject()) {
                count += 1;
            }
            
            file.close();
        }
    }
    
    // 递归统计子目录
    QFileInfoList subDirs = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const auto &subDirInfo : subDirs) {
        count += countQuestionsInDirectory(subDirInfo.absoluteFilePath());
    }
    
    return count;
}

void QuestionBankPanel::onLoadBank()
{
    QListWidgetItem *item = m_questionList->currentItem();
    if (!item) {
        return;
    }
    
    // 获取题库ID并切换
    QString bankId = item->data(Qt::UserRole).toString();
    
    if (QuestionBankManager::instance().switchToBank(bankId)) {
        QuestionBankInfo info = QuestionBankManager::instance().getBankInfo(bankId);
        m_selectedBankPath = info.path;
        
        emit bankLoadRequested(m_selectedBankPath);
        
        // 刷新列表以更新当前题库标记
        refreshBankList();
    } else {
        QMessageBox::warning(this, "切换失败", "无法切换到该题库");
    }
}

void QuestionBankPanel::onDeleteBank()
{
    QListWidgetItem *item = m_questionList->currentItem();
    if (!item) {
        return;
    }
    
    // 获取题库ID
    QString bankId = item->data(Qt::UserRole).toString();
    QuestionBankInfo info = QuestionBankManager::instance().getBankInfo(bankId);
    
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "确认删除",
        QString("确定要删除题库【%1】吗？\n\n"
                "此操作将从题库管理器中注销该题库。\n"
                "注意：不会删除实际文件。").arg(info.name),
        QMessageBox::Yes | QMessageBox::No
    );
    
    if (reply == QMessageBox::Yes) {
        // 使用 QuestionBankManager 删除（只注销，不删文件）
        if (QuestionBankManager::instance().deleteQuestionBank(bankId)) {
            QMessageBox::information(this, "删除成功", 
                QString("题库【%1】已从管理器中注销").arg(info.name));
            
            refreshBankList();
        } else {
            QMessageBox::warning(this, "删除失败", 
                QString("无法删除题库【%1】").arg(info.name));
        }
    }
}

void QuestionBankPanel::onRenameBank()
{
    QListWidgetItem *item = m_questionList->currentItem();
    if (!item) {
        return;
    }
    
    // 获取题库ID
    QString bankId = item->data(Qt::UserRole).toString();
    QuestionBankInfo info = QuestionBankManager::instance().getBankInfo(bankId);
    
    bool ok;
    QString newName = QInputDialog::getText(
        this,
        "重命名题库",
        "请输入新的题库名称：",
        QLineEdit::Normal,
        info.name,
        &ok
    );
    
    if (ok && !newName.isEmpty() && newName != info.name) {
        // 使用 QuestionBankManager 重命名
        if (QuestionBankManager::instance().renameQuestionBank(bankId, newName)) {
            QMessageBox::information(this, "重命名成功", 
                QString("题库已重命名为【%1】").arg(newName));
            
            refreshBankList();
        } else {
            QMessageBox::warning(this, "重命名失败", 
                "无法重命名题库");
        }
    }
}

void QuestionBankPanel::updateQuestionStatus(const QString &questionId)
{
    if (m_mode != PanelMode::Questions) return;
    
    for (int i = 0; i < m_questionList->count(); ++i) {
        QListWidgetItem *item = m_questionList->item(i);
        int index = item->data(Qt::UserRole).toInt();
        if (index >= 0 && index < m_allQuestions.size()) {
            const Question &q = m_allQuestions[index];
            if (q.id() == questionId) {
                QString difficultyText;
                switch (q.difficulty()) {
                    case Difficulty::Easy:
                        difficultyText = "简单";
                        break;
                    case Difficulty::Medium:
                        difficultyText = "中等";
                        break;
                    case Difficulty::Hard:
                        difficultyText = "困难";
                        break;
                }
                
                QString statusIcon = getQuestionStatusIcon(questionId);
                
                QString itemText = QString("%1 %2. %3 [%4]")
                    .arg(statusIcon)
                    .arg(index + 1)
                    .arg(q.title())
                    .arg(difficultyText);
                
                item->setText(itemText);
                break;
            }
        }
    }
}

QString QuestionBankPanel::getQuestionStatusIcon(const QString &questionId) const
{
    QuestionProgressRecord progress = ProgressManager::instance().getProgress(questionId);
    
    switch (progress.status) {
        case QuestionStatus::NotStarted:
            return QString::fromUtf8("\xE2\x9A\xAA");  // ⚪ 未开始
        case QuestionStatus::InProgress:
            return QString::fromUtf8("\xF0\x9F\x94\xB5");  // 🔵 进行中
        case QuestionStatus::Completed:
            return QString::fromUtf8("\xE2\x9C\x85");  // ✅ 已完成
        case QuestionStatus::Mastered:
            return QString::fromUtf8("\xF0\x9F\x8F\x86");  // 🏆 已掌握
        default:
            return QString::fromUtf8("\xE2\x9A\xAA");  // ⚪
    }
}
