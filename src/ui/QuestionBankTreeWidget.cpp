#include "QuestionBankTreeWidget.h"
#include "QuestionEditorDialog.h"
#include "../core/QuestionBankManager.h"
#include "../core/ProgressManager.h"
#include "../utils/OperationHistory.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QHeaderView>
#include <QDebug>
#include <QMenu>
#include <QMessageBox>
#include <QRegularExpression>

QuestionBankTreeWidget::QuestionBankTreeWidget(QWidget *parent)
    : QTreeWidget(parent)
    , m_rootItem(nullptr)
{
    setupUI();
    // 注意：不在构造函数中加载树，等待筛选状态恢复后再加载
    // loadBankTree() 会在 MainWindow 中调用 refreshBankTree() 时执行
}

void QuestionBankTreeWidget::setupUI()
{
    // 设置列
    setColumnCount(1);
    setHeaderLabel("题库列表");
    
    // 样式
    setStyleSheet(R"(
        QTreeWidget {
            background-color: #2d2d2d;
            color: #e8e8e8;
            border: 1px solid #3a3a3a;
            border-radius: 4px;
            outline: none;
        }
        QTreeWidget::item {
            padding: 6px;
            border: none;
            outline: none;
        }
        QTreeWidget::item:selected {
            background-color: #660000;
            color: #ffffff;
        }
        QTreeWidget::item:selected:hover {
            background-color: #880000;  /* 更浅的红色 */
            color: #ffffff;
        }
        QTreeWidget::item:hover {
            background-color: #323232;
        }
        QTreeWidget::branch {
            background-color: #2d2d2d;
        }
        QTreeWidget::branch:has-children:!has-siblings:closed,
        QTreeWidget::branch:closed:has-children:has-siblings {
            image: url(:/icons/branch-closed.png);
        }
        QTreeWidget::branch:open:has-children:!has-siblings,
        QTreeWidget::branch:open:has-children:has-siblings {
            image: url(:/icons/branch-open.png);
        }
    )");
    
    // 设置属性
    setAnimated(true);
    setIndentation(15);  // 减少缩进，避免水平移动太多
    setUniformRowHeights(false);
    setExpandsOnDoubleClick(false);  // 禁用双击展开，使用单击
    setSelectionMode(QAbstractItemView::SingleSelection);
    setFocusPolicy(Qt::StrongFocus);
    
    // 启用右键菜单
    setContextMenuPolicy(Qt::CustomContextMenu);
    
    // 连接信号
    connect(this, &QTreeWidget::itemClicked,
            this, &QuestionBankTreeWidget::onItemClicked);
    connect(this, &QTreeWidget::itemDoubleClicked,
            this, &QuestionBankTreeWidget::onItemDoubleClicked);
    connect(this, &QTreeWidget::customContextMenuRequested,
            this, &QuestionBankTreeWidget::onCustomContextMenu);
}

void QuestionBankTreeWidget::loadBankTree()
{
    clear();
    m_rootItem = nullptr;
    
    loadRootNode();
}

void QuestionBankTreeWidget::loadRootNode()
{
    // 创建根节点
    m_rootItem = new QTreeWidgetItem(this);
    m_rootItem->setText(0, "📁 基础题库");
    m_rootItem->setData(0, Qt::UserRole, static_cast<int>(TreeNodeType::Root));
    m_rootItem->setData(0, Qt::UserRole + 1, "data/基础题库");
    m_rootItem->setExpanded(true);
    
    // 加载所有题库
    QDir baseDir("data/基础题库");
    if (!baseDir.exists()) {
        qWarning() << "基础题库目录不存在";
        return;
    }
    
    QStringList banks = baseDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    
    for (const QString &bankName : banks) {
        QString bankPath = baseDir.filePath(bankName);
        loadBankNode(m_rootItem, bankPath);
    }
    
    qDebug() << "加载了" << banks.size() << "个题库";
}

void QuestionBankTreeWidget::loadBankNode(QTreeWidgetItem *parentItem, const QString &bankPath)
{
    QFileInfo pathInfo(bankPath);
    QString bankName = pathInfo.fileName();
    
    // 统计题目数量
    int questionCount = countQuestionsInBank(bankPath);
    
    // 创建题库节点
    QTreeWidgetItem *bankItem = new QTreeWidgetItem(parentItem);
    bankItem->setText(0, QString("📚 %1 (%2 道题目)").arg(bankName).arg(questionCount));
    bankItem->setData(0, Qt::UserRole, static_cast<int>(TreeNodeType::Bank));
    bankItem->setData(0, Qt::UserRole + 1, bankPath);
    
    // 加载题目文件
    loadQuestionFiles(bankItem, bankPath);
}

void QuestionBankTreeWidget::loadQuestionFiles(QTreeWidgetItem *bankItem, const QString &bankPath)
{
    QDir bankDir(bankPath);
    if (!bankDir.exists()) {
        return;
    }
    
    // 加载 JSON 文件
    QStringList filters;
    filters << "*.json";
    QFileInfoList files = bankDir.entryInfoList(filters, QDir::Files, QDir::Name);
    
    for (const QFileInfo &fileInfo : files) {
        QString fileName = fileInfo.fileName();
        QString filePath = fileInfo.absoluteFilePath();
        
        // 移除 .json 后缀
        QString displayName = fileName;
        if (displayName.endsWith(".json", Qt::CaseInsensitive)) {
            displayName.chop(5);
        }
        
        // 加载题目以获取ID和状态
        Question question = loadQuestionFromFile(filePath);
        
        // 应用难度筛选
        if (!shouldShowQuestion(question)) {
            continue;  // 跳过不符合筛选条件的题目
        }
        
        QString statusIcon = getQuestionStatusIcon(question.id());
        
        // 创建题目节点
        QTreeWidgetItem *questionItem = new QTreeWidgetItem(bankItem);
        questionItem->setText(0, QString("%1 %2").arg(statusIcon).arg(displayName));
        questionItem->setData(0, Qt::UserRole, static_cast<int>(TreeNodeType::QuestionFile));
        questionItem->setData(0, Qt::UserRole + 1, filePath);
        questionItem->setData(0, Qt::UserRole + 2, question.id());  // 保存题目ID
    }
    
    // 递归加载子目录
    QFileInfoList subDirs = bankDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    for (const QFileInfo &subDirInfo : subDirs) {
        QString subDirPath = subDirInfo.absoluteFilePath();
        QString subDirName = subDirInfo.fileName();
        
        // 创建子目录节点
        QTreeWidgetItem *subDirItem = new QTreeWidgetItem(bankItem);
        subDirItem->setText(0, QString("📁 %1").arg(subDirName));
        subDirItem->setData(0, Qt::UserRole, static_cast<int>(TreeNodeType::Bank));
        subDirItem->setData(0, Qt::UserRole + 1, subDirPath);
        
        // 递归加载子目录的题目
        loadQuestionFiles(subDirItem, subDirPath);
    }
}

int QuestionBankTreeWidget::countQuestionsInBank(const QString &bankPath) const
{
    int count = 0;
    QDir dir(bankPath);
    
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
        count += countQuestionsInBank(subDirInfo.absoluteFilePath());
    }
    
    return count;
}

Question QuestionBankTreeWidget::loadQuestionFromFile(const QString &filePath) const
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "无法打开题目文件:" << filePath;
        return Question();
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    
    if (doc.isObject()) {
        return Question(doc.object());
    } else if (doc.isArray() && doc.array().size() > 0) {
        return Question(doc.array().first().toObject());
    }
    
    return Question();
}

void QuestionBankTreeWidget::onItemClicked(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column);
    
    if (!item) return;
    
    TreeNodeType type = getNodeType(item);
    QString path = getNodePath(item);
    
    if (type == TreeNodeType::QuestionFile) {
        // 加载并发出题目选中信号
        Question question = loadQuestionFromFile(path);
        if (!question.id().isEmpty()) {
            emit questionSelected(path, question);
        }
        
        // 添加视觉反馈
        item->setSelected(true);
    } else if (type == TreeNodeType::Bank || type == TreeNodeType::Root) {
        // 单击题库或文件夹时展开/折叠
        item->setExpanded(!item->isExpanded());
        emit bankSelected(path);
    }
}

void QuestionBankTreeWidget::onItemDoubleClicked(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column);
    
    if (!item) return;
    
    TreeNodeType type = getNodeType(item);
    
    // 双击题目文件时也加载（增强响应）
    if (type == TreeNodeType::QuestionFile) {
        QString path = getNodePath(item);
        Question question = loadQuestionFromFile(path);
        if (!question.id().isEmpty()) {
            emit questionSelected(path, question);
        }
    }
}

TreeNodeType QuestionBankTreeWidget::getNodeType(QTreeWidgetItem *item) const
{
    if (!item) return TreeNodeType::Root;
    
    int typeValue = item->data(0, Qt::UserRole).toInt();
    return static_cast<TreeNodeType>(typeValue);
}

QString QuestionBankTreeWidget::getNodePath(QTreeWidgetItem *item) const
{
    if (!item) return QString();
    
    return item->data(0, Qt::UserRole + 1).toString();
}

void QuestionBankTreeWidget::expandBank(const QString &bankPath)
{
    if (!m_rootItem) return;
    
    // 遍历查找对应的题库节点
    for (int i = 0; i < m_rootItem->childCount(); ++i) {
        QTreeWidgetItem *bankItem = m_rootItem->child(i);
        QString itemPath = getNodePath(bankItem);
        
        if (itemPath == bankPath) {
            bankItem->setExpanded(true);
            scrollToItem(bankItem);
            return;
        }
    }
}

void QuestionBankTreeWidget::selectQuestion(const QString &questionPath)
{
    if (!m_rootItem) return;
    
    // 递归查找题目节点
    std::function<bool(QTreeWidgetItem*)> findAndSelect = [&](QTreeWidgetItem *parent) -> bool {
        for (int i = 0; i < parent->childCount(); ++i) {
            QTreeWidgetItem *child = parent->child(i);
            QString itemPath = getNodePath(child);
            
            if (itemPath == questionPath) {
                setCurrentItem(child);
                scrollToItem(child);
                return true;
            }
            
            if (findAndSelect(child)) {
                return true;
            }
        }
        return false;
    };
    
    findAndSelect(m_rootItem);
}

void QuestionBankTreeWidget::refreshTree()
{
    // 保存当前展开状态
    QSet<QString> expandedPaths;
    
    std::function<void(QTreeWidgetItem*)> saveExpandedState = [&](QTreeWidgetItem *item) {
        if (item && item->isExpanded()) {
            expandedPaths.insert(getNodePath(item));
        }
        for (int i = 0; i < item->childCount(); ++i) {
            saveExpandedState(item->child(i));
        }
    };
    
    if (m_rootItem) {
        saveExpandedState(m_rootItem);
    }
    
    // 重新加载
    loadBankTree();
    
    // 恢复展开状态
    std::function<void(QTreeWidgetItem*)> restoreExpandedState = [&](QTreeWidgetItem *item) {
        QString path = getNodePath(item);
        if (expandedPaths.contains(path)) {
            item->setExpanded(true);
        }
        for (int i = 0; i < item->childCount(); ++i) {
            restoreExpandedState(item->child(i));
        }
    };
    
    if (m_rootItem) {
        restoreExpandedState(m_rootItem);
    }
}

QString QuestionBankTreeWidget::getQuestionStatusIcon(const QString &questionId) const
{
    if (questionId.isEmpty()) {
        return "⚪";  // 未知状态
    }
    
    QuestionProgressRecord progress = ProgressManager::instance().getProgress(questionId);
    
    switch (progress.status) {
        case QuestionStatus::NotStarted:
            return "⚪";  // 未开始
        case QuestionStatus::InProgress:
            return "🔵";  // 进行中
        case QuestionStatus::Completed:
            return "✅";  // 已完成
        case QuestionStatus::Mastered:
            return "⭐";  // 已掌握
        default:
            return "⚪";
    }
}

void QuestionBankTreeWidget::updateQuestionStatus(const QString &questionId)
{
    if (questionId.isEmpty()) return;
    
    // 递归查找并更新题目节点
    std::function<void(QTreeWidgetItem*)> updateNode = [&](QTreeWidgetItem *item) {
        if (!item) return;
        
        // 检查是否是题目节点
        TreeNodeType type = getNodeType(item);
        if (type == TreeNodeType::QuestionFile) {
            QString itemQuestionId = item->data(0, Qt::UserRole + 2).toString();
            if (itemQuestionId == questionId) {
                // 更新状态图标
                QString text = item->text(0);
                QString statusIcon = getQuestionStatusIcon(questionId);
                
                // 移除旧的状态图标，添加新的
                QStringList parts = text.split(" ", Qt::SkipEmptyParts);
                if (parts.size() >= 2) {
                    parts[0] = statusIcon;
                    item->setText(0, parts.join(" "));
                }
            }
        }
        
        // 递归处理子节点
        for (int i = 0; i < item->childCount(); ++i) {
            updateNode(item->child(i));
        }
    };
    
    if (m_rootItem) {
        updateNode(m_rootItem);
    }
}


QStringList QuestionBankTreeWidget::getExpandedPaths() const
{
    QStringList expandedPaths;
    
    std::function<void(QTreeWidgetItem*)> collectExpanded = [&](QTreeWidgetItem *item) {
        if (!item) return;
        
        if (item->isExpanded()) {
            QString path = getNodePath(item);
            if (!path.isEmpty()) {
                expandedPaths.append(path);
            }
        }
        
        for (int i = 0; i < item->childCount(); ++i) {
            collectExpanded(item->child(i));
        }
    };
    
    if (m_rootItem) {
        collectExpanded(m_rootItem);
    }
    
    return expandedPaths;
}

void QuestionBankTreeWidget::restoreExpandedPaths(const QStringList &paths)
{
    if (paths.isEmpty()) return;
    
    QSet<QString> pathSet = QSet<QString>(paths.begin(), paths.end());
    
    std::function<void(QTreeWidgetItem*)> expandItems = [&](QTreeWidgetItem *item) {
        if (!item) return;
        
        QString path = getNodePath(item);
        if (pathSet.contains(path)) {
            item->setExpanded(true);
        }
        
        for (int i = 0; i < item->childCount(); ++i) {
            expandItems(item->child(i));
        }
    };
    
    if (m_rootItem) {
        expandItems(m_rootItem);
    }
}

QString QuestionBankTreeWidget::getSelectedQuestionPath() const
{
    QTreeWidgetItem *item = currentItem();
    if (!item) return QString();
    
    TreeNodeType type = getNodeType(item);
    if (type == TreeNodeType::QuestionFile) {
        return getNodePath(item);
    }
    
    return QString();
}

void QuestionBankTreeWidget::onCustomContextMenu(const QPoint &pos)
{
    QTreeWidgetItem *item = itemAt(pos);
    if (!item) {
        return;
    }
    
    TreeNodeType type = getNodeType(item);
    
    QMenu menu(this);
    menu.setStyleSheet(R"(
        QMenu {
            background-color: #2d2d2d;
            color: #e8e8e8;
            border: 1px solid #3a3a3a;
        }
        QMenu::item {
            padding: 8px 20px;
        }
        QMenu::item:selected {
            background-color: #660000;
        }
        QMenu::separator {
            height: 1px;
            background-color: #3a3a3a;
            margin: 4px 0;
        }
    )");
    
    if (type == TreeNodeType::Bank) {
        // 题库文件夹右键菜单
        QAction *addAction = menu.addAction("➕ 新建题目");
        menu.addSeparator();
        QAction *deleteAction = menu.addAction("🗑️ 删除题库");
        
        QAction *selected = menu.exec(mapToGlobal(pos));
        
        if (selected == addAction) {
            onAddQuestion();
        } else if (selected == deleteAction) {
            onDeleteBank();
        }
    } else if (type == TreeNodeType::QuestionFile) {
        // 题目文件右键菜单
        QAction *editAction = menu.addAction("✏️ 编辑题目");
        menu.addSeparator();
        QAction *deleteAction = menu.addAction("🗑️ 删除题目");
        
        QAction *selected = menu.exec(mapToGlobal(pos));
        
        if (selected == editAction) {
            onEditQuestion();
        } else if (selected == deleteAction) {
            onDeleteQuestion();
        }
    } else if (type == TreeNodeType::Root) {
        // 根节点右键菜单
        QAction *addAction = menu.addAction("➕ 新建题库");
        menu.exec(mapToGlobal(pos));
    }
}

void QuestionBankTreeWidget::onAddQuestion()
{
    QTreeWidgetItem *item = currentItem();
    if (!item) return;
    
    TreeNodeType type = getNodeType(item);
    if (type != TreeNodeType::Bank) {
        return;
    }
    
    QString bankPath = getNodePath(item);
    
    // 显示选择对话框：手动输入 or 文件导入
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("新建题目");
    msgBox.setText("请选择创建方式：");
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setStyleSheet(R"(
        QMessageBox {
            background-color: #242424;
        }
        QMessageBox QLabel {
            color: #e8e8e8;
        }
        QPushButton {
            background-color: #3a3a3a;
            color: white;
            border: none;
            border-radius: 4px;
            padding: 8px 16px;
            min-width: 80px;
        }
        QPushButton:hover {
            background-color: #4a4a4a;
        }
    )");
    
    QPushButton *manualBtn = msgBox.addButton("✏️ 手动输入", QMessageBox::ActionRole);
    QPushButton *importBtn = msgBox.addButton("📁 文件导入", QMessageBox::ActionRole);
    QPushButton *cancelBtn = msgBox.addButton("取消", QMessageBox::RejectRole);
    
    msgBox.exec();
    
    if (msgBox.clickedButton() == manualBtn) {
        // 手动输入模式
        QuestionEditorDialog *dialog = new QuestionEditorDialog(QuestionEditorDialog::CreateMode, this);
        if (dialog->exec() == QDialog::Accepted) {
            Question newQuestion = dialog->getQuestion();
            
            // 保存题目到文件
            QString fileName = newQuestion.title();
            fileName.replace(QRegularExpression("[\\\\/:*?\"<>|]"), "_");
            QString filePath = bankPath + "/" + fileName + ".json";
            
            QFile file(filePath);
            if (file.open(QIODevice::WriteOnly)) {
                QJsonDocument doc(newQuestion.toJson());
                file.write(doc.toJson(QJsonDocument::Indented));
                file.close();
                
                // 刷新树
                refreshTree();
                
                QMessageBox::information(this, "成功", "题目创建成功！");
            } else {
                QMessageBox::warning(this, "错误", "无法保存题目文件");
            }
        }
        delete dialog;
    } else if (msgBox.clickedButton() == importBtn) {
        // 文件导入模式
        QuestionEditorDialog *dialog = new QuestionEditorDialog(QuestionEditorDialog::ImportMode, this);
        dialog->onImportFromFile();  // 直接触发导入
        
        if (dialog->exec() == QDialog::Accepted) {
            Question newQuestion = dialog->getQuestion();
            
            // 保存题目到文件
            QString fileName = newQuestion.title();
            fileName.replace(QRegularExpression("[\\\\/:*?\"<>|]"), "_");
            QString filePath = bankPath + "/" + fileName + ".json";
            
            QFile file(filePath);
            if (file.open(QIODevice::WriteOnly)) {
                QJsonDocument doc(newQuestion.toJson());
                file.write(doc.toJson(QJsonDocument::Indented));
                file.close();
                
                // 刷新树
                refreshTree();
                
                QMessageBox::information(this, "成功", "题目导入成功！");
            } else {
                QMessageBox::warning(this, "错误", "无法保存题目文件");
            }
        }
        delete dialog;
    }
}

void QuestionBankTreeWidget::onEditQuestion()
{
    QTreeWidgetItem *item = currentItem();
    if (!item) return;
    
    TreeNodeType type = getNodeType(item);
    if (type != TreeNodeType::QuestionFile) {
        return;
    }
    
    QString filePath = getNodePath(item);
    Question question = loadQuestionFromFile(filePath);
    
    if (question.id().isEmpty()) {
        QMessageBox::warning(this, "错误", "无法加载题目");
        return;
    }
    
    QuestionEditorDialog *dialog = new QuestionEditorDialog(question, this);
    if (dialog->exec() == QDialog::Accepted) {
        Question updatedQuestion = dialog->getQuestion();
        
        // 保存更新后的题目
        QFile file(filePath);
        if (file.open(QIODevice::WriteOnly)) {
            QJsonDocument doc(updatedQuestion.toJson());
            file.write(doc.toJson(QJsonDocument::Indented));
            file.close();
            
            // 刷新树
            refreshTree();
            
            QMessageBox::information(this, "成功", "题目已更新！");
        } else {
            QMessageBox::warning(this, "错误", "无法保存题目文件");
        }
    }
    delete dialog;
}

void QuestionBankTreeWidget::onDeleteQuestion()
{
    QTreeWidgetItem *item = currentItem();
    if (!item) return;
    
    TreeNodeType type = getNodeType(item);
    if (type != TreeNodeType::QuestionFile) {
        return;
    }
    
    QString filePath = getNodePath(item);
    QString fileName = QFileInfo(filePath).fileName();
    
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "确认删除",
        QString("确定要删除题目【%1】吗？\n\n此操作可以通过 Ctrl+Z 撤销。").arg(fileName),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
    );
    
    if (reply == QMessageBox::Yes) {
        // 读取文件内容用于撤销
        QFile file(filePath);
        QByteArray content;
        if (file.open(QIODevice::ReadOnly)) {
            content = file.readAll();
            file.close();
        }
        
        // 使用 OperationHistory 删除（移动到回收站）
        OperationHistory::instance().recordDeleteQuestion(filePath, content);
        
        refreshTree();
        QMessageBox::information(this, "成功", "题目已删除\n\n按 Ctrl+Z 可撤销此操作");
    }
}

void QuestionBankTreeWidget::onDeleteBank()
{
    QTreeWidgetItem *item = currentItem();
    if (!item) return;
    
    TreeNodeType type = getNodeType(item);
    if (type != TreeNodeType::Bank) {
        return;
    }
    
    QString bankPath = getNodePath(item);
    QString bankName = QFileInfo(bankPath).fileName();
    
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "确认删除",
        QString("确定要删除题库【%1】吗？\n\n此操作将删除该题库下的所有题目！\n此操作可以通过 Ctrl+Z 撤销。").arg(bankName),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
    );
    
    if (reply == QMessageBox::Yes) {
        // 使用 OperationHistory 删除（移动到回收站）
        OperationHistory::instance().recordDeleteBank(bankPath);
        
        refreshTree();
        QMessageBox::information(this, "成功", "题库已删除\n\n按 Ctrl+Z 可撤销此操作");
    }
}


void QuestionBankTreeWidget::setDifficultyFilter(const QSet<Difficulty> &difficulties)
{
    m_difficultyFilter = difficulties;
    
    qDebug() << "[QuestionBankTreeWidget] Difficulty filter set. Active filters:" << m_difficultyFilter.size();
    
    // 重新加载树以应用筛选
    refreshTree();
}

bool QuestionBankTreeWidget::shouldShowQuestion(const Question &question) const
{
    // 如果没有设置筛选（空集合），显示所有题目
    if (m_difficultyFilter.isEmpty()) {
        return true;
    }
    
    // 检查题目难度是否在筛选列表中
    return m_difficultyFilter.contains(question.difficulty());
}
