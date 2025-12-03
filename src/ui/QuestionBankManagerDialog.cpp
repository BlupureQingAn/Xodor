#include "QuestionBankManagerDialog.h"
#include "AIImportDialog.h"
#include "ImportDialog.h"
#include "../ai/OllamaClient.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QInputDialog>
#include <QFileDialog>

QuestionBankManagerDialog::QuestionBankManagerDialog(OllamaClient *aiClient, QWidget *parent)
    : QDialog(parent)
    , m_aiClient(aiClient)
{
    setupUI();
    refreshBankList();
    
    setWindowTitle("题库管理");
    resize(800, 600);
    
    // 连接信号
    connect(&QuestionBankManager::instance(), &QuestionBankManager::bankListChanged,
            this, &QuestionBankManagerDialog::onBankListChanged);
}

void QuestionBankManagerDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(16);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    
    // 标题
    QLabel *titleLabel = new QLabel("📚 题库管理", this);
    titleLabel->setStyleSheet("font-size: 18pt; font-weight: bold; color: #e8e8e8;");
    
    // 信息标签
    m_infoLabel = new QLabel("选择一个题库进行操作", this);
    m_infoLabel->setStyleSheet("color: #b0b0b0; font-size: 10pt;");
    
    // 题库列表
    m_bankList = new QListWidget(this);
    m_bankList->setStyleSheet(R"(
        QListWidget {
            background-color: #242424;
            color: #e8e8e8;
            border: 1px solid #4a4a4a;
            border-radius: 8px;
            padding: 8px;
        }
        QListWidget::item {
            padding: 12px;
            border-radius: 6px;
            margin: 4px 0;
        }
        QListWidget::item:hover {
            background-color: #2d2d2d;
        }
        QListWidget::item:selected {
            background-color: #660000;
            color: white;
        }
    )");
    
    connect(m_bankList, &QListWidget::itemSelectionChanged,
            this, &QuestionBankManagerDialog::onBankSelectionChanged);
    connect(m_bankList, &QListWidget::itemDoubleClicked,
            this, &QuestionBankManagerDialog::onSwitchBank);
    
    // 操作按钮
    QHBoxLayout *btnLayout1 = new QHBoxLayout();
    
    m_switchBtn = new QPushButton("✓ 切换到此题库", this);
    m_deleteBtn = new QPushButton("🗑️ 删除题库", this);
    m_refreshBtn = new QPushButton("🔄 刷新题库", this);
    m_renameBtn = new QPushButton("✏️ 重命名", this);
    
    btnLayout1->addWidget(m_switchBtn);
    btnLayout1->addWidget(m_deleteBtn);
    btnLayout1->addWidget(m_refreshBtn);
    btnLayout1->addWidget(m_renameBtn);
    
    // 底部按钮
    QHBoxLayout *btnLayout2 = new QHBoxLayout();
    
    m_importBtn = new QPushButton("➕ 导入新题库", this);
    m_closeBtn = new QPushButton("关闭", this);
    
    btnLayout2->addWidget(m_importBtn);
    btnLayout2->addStretch();
    btnLayout2->addWidget(m_closeBtn);
    
    // 按钮样式
    QString btnStyle = R"(
        QPushButton {
            background-color: #660000;
            color: white;
            border: none;
            border-radius: 8px;
            padding: 10px 20px;
            font-weight: 500;
        }
        QPushButton:hover {
            background-color: #880000;
        }
        QPushButton:pressed {
            background-color: #440000;
        }
        QPushButton:disabled {
            background-color: #3a3a3a;
            color: #707070;
        }
    )";
    
    m_switchBtn->setStyleSheet(btnStyle);
    m_deleteBtn->setStyleSheet(btnStyle);
    m_refreshBtn->setStyleSheet(btnStyle);
    m_renameBtn->setStyleSheet(btnStyle);
    m_importBtn->setStyleSheet(btnStyle);
    m_closeBtn->setStyleSheet(btnStyle);
    
    // 布局
    mainLayout->addWidget(titleLabel);
    mainLayout->addWidget(m_infoLabel);
    mainLayout->addWidget(m_bankList);
    mainLayout->addLayout(btnLayout1);
    mainLayout->addLayout(btnLayout2);
    
    // 连接信号
    connect(m_switchBtn, &QPushButton::clicked, this, &QuestionBankManagerDialog::onSwitchBank);
    connect(m_deleteBtn, &QPushButton::clicked, this, &QuestionBankManagerDialog::onDeleteBank);
    connect(m_refreshBtn, &QPushButton::clicked, this, &QuestionBankManagerDialog::onRefreshBank);
    connect(m_renameBtn, &QPushButton::clicked, this, &QuestionBankManagerDialog::onRenameBank);
    connect(m_importBtn, &QPushButton::clicked, this, &QuestionBankManagerDialog::onImportNewBank);
    connect(m_closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    
    // 应用对话框样式
    setStyleSheet(R"(
        QDialog {
            background-color: #242424;
        }
        QLabel {
            color: #e8e8e8;
        }
    )");
    
    updateButtons();
}

void QuestionBankManagerDialog::refreshBankList()
{
    m_bankList->clear();
    
    QVector<QuestionBankInfo> banks = QuestionBankManager::instance().getAllBanks();
    QString currentBankId = QuestionBankManager::instance().getCurrentBankId();
    
    if (banks.isEmpty()) {
        m_infoLabel->setText("暂无题库，点击\"导入新题库\"开始");
        return;
    }
    
    m_infoLabel->setText(QString("共有 %1 个题库").arg(banks.size()));
    
    for (const QuestionBankInfo &info : banks) {
        QString text = formatBankInfo(info);
        QListWidgetItem *item = new QListWidgetItem(text);
        item->setData(Qt::UserRole, info.id);
        
        // 标记当前题库
        if (info.id == currentBankId) {
            QFont font = item->font();
            font.setBold(true);
            item->setFont(font);
            item->setText("⭐ " + text);
        }
        
        m_bankList->addItem(item);
    }
}

QString QuestionBankManagerDialog::formatBankInfo(const QuestionBankInfo &info) const
{
    QString text = QString("%1\n").arg(info.name);
    text += QString("  📊 %1 道题目 | ").arg(info.questionCount);
    text += QString("📅 导入于 %1").arg(info.importTime.toString("yyyy-MM-dd hh:mm"));
    
    if (info.isAIParsed) {
        text += " | 🤖 AI解析";
    }
    
    return text;
}

void QuestionBankManagerDialog::onBankListChanged()
{
    refreshBankList();
}

void QuestionBankManagerDialog::onBankSelectionChanged()
{
    QListWidgetItem *item = m_bankList->currentItem();
    if (item) {
        m_selectedBankId = item->data(Qt::UserRole).toString();
    } else {
        m_selectedBankId.clear();
    }
    
    updateButtons();
}

void QuestionBankManagerDialog::updateButtons()
{
    bool hasSelection = !m_selectedBankId.isEmpty();
    bool isCurrent = (m_selectedBankId == QuestionBankManager::instance().getCurrentBankId());
    
    m_switchBtn->setEnabled(hasSelection && !isCurrent);
    m_deleteBtn->setEnabled(hasSelection);
    m_refreshBtn->setEnabled(hasSelection);
    m_renameBtn->setEnabled(hasSelection);
}

void QuestionBankManagerDialog::onSwitchBank()
{
    if (m_selectedBankId.isEmpty()) return;
    
    if (QuestionBankManager::instance().switchToBank(m_selectedBankId)) {
        emit bankSelected(m_selectedBankId);
        
        QuestionBankInfo info = QuestionBankManager::instance().getBankInfo(m_selectedBankId);
        QMessageBox::information(this, "切换成功",
            QString("已切换到题库：%1\n\n"
                    "题目数量：%2 道").arg(info.name).arg(info.questionCount));
        
        refreshBankList();
        updateButtons();
    }
}

void QuestionBankManagerDialog::onDeleteBank()
{
    if (m_selectedBankId.isEmpty()) return;
    
    QuestionBankInfo info = QuestionBankManager::instance().getBankInfo(m_selectedBankId);
    
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("确认删除");
    msgBox.setText(QString("确定要删除题库\"%1\"吗？").arg(info.name));
    msgBox.setInformativeText(QString("此操作将删除程序内部的题库副本（%1 道题目）。\n\n"
                                     "注意：不会删除原始导入文件夹。")
                             .arg(info.questionCount));
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);
    
    if (msgBox.exec() == QMessageBox::Yes) {
        if (QuestionBankManager::instance().deleteQuestionBank(m_selectedBankId)) {
            emit bankDeleted(m_selectedBankId);
            
            QMessageBox::information(this, "删除成功",
                QString("题库\"%1\"已删除").arg(info.name));
            
            m_selectedBankId.clear();
            refreshBankList();
            updateButtons();
        } else {
            QMessageBox::critical(this, "删除失败",
                "删除题库时发生错误");
        }
    }
}

void QuestionBankManagerDialog::onRefreshBank()
{
    if (m_selectedBankId.isEmpty()) return;
    
    QuestionBankInfo info = QuestionBankManager::instance().getBankInfo(m_selectedBankId);
    
    // 询问是否使用AI重新解析
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("刷新题库");
    msgBox.setText(QString("如何刷新题库\"%1\"？").arg(info.name));
    msgBox.setIcon(QMessageBox::Question);
    
    QPushButton *aiBtn = msgBox.addButton("🤖 AI重新解析", QMessageBox::AcceptRole);
    QPushButton *normalBtn = msgBox.addButton("📁 普通刷新", QMessageBox::ActionRole);
    QPushButton *cancelBtn = msgBox.addButton("取消", QMessageBox::RejectRole);
    
    msgBox.setDefaultButton(aiBtn);
    msgBox.exec();
    
    if (msgBox.clickedButton() == cancelBtn) {
        return;
    }
    
    bool useAI = (msgBox.clickedButton() == aiBtn);
    
    // TODO: 实现刷新逻辑
    emit bankRefreshed(m_selectedBankId);
    
    QMessageBox::information(this, "刷新完成",
        QString("题库\"%1\"已刷新").arg(info.name));
}

void QuestionBankManagerDialog::onRenameBank()
{
    if (m_selectedBankId.isEmpty()) return;
    
    QuestionBankInfo info = QuestionBankManager::instance().getBankInfo(m_selectedBankId);
    
    bool ok;
    QString newName = QInputDialog::getText(this, "重命名题库",
                                           "新名称:", QLineEdit::Normal,
                                           info.name, &ok);
    
    if (ok && !newName.isEmpty() && newName != info.name) {
        if (QuestionBankManager::instance().renameQuestionBank(m_selectedBankId, newName)) {
            QMessageBox::information(this, "重命名成功",
                QString("题库已重命名为：%1").arg(newName));
            
            refreshBankList();
        }
    }
}

void QuestionBankManagerDialog::onImportNewBank()
{
    // 关闭当前对话框，让MainWindow处理导入
    accept();
    // 发送信号通知需要导入
    emit bankSelected(QString());  // 空ID表示需要导入新题库
}
