#include "ChatHistoryDialog.h"
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMessageBox>
#include <QGroupBox>

ChatHistoryDialog::ChatHistoryDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("对话历史记录");
    setMinimumSize(600, 400);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // 信息标签
    m_infoLabel = new QLabel("选择要加载或删除的对话记录：", this);
    mainLayout->addWidget(m_infoLabel);
    
    // 列表
    m_listWidget = new QListWidget(this);
    m_listWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    m_listWidget->setStyleSheet(R"(
        QListWidget {
            background-color: #2a2a2a;
            border: 1px solid #444;
            border-radius: 4px;
            padding: 4px;
        }
        QListWidget::item {
            padding: 8px;
            border-radius: 4px;
            margin: 2px;
        }
        QListWidget::item:selected {
            background-color: #3b82f6;
            color: white;
        }
        QListWidget::item:hover {
            background-color: #374151;
        }
    )");
    mainLayout->addWidget(m_listWidget);
    
    // 按钮
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    
    m_loadButton = new QPushButton("📂 加载", this);
    m_loadButton->setEnabled(false);
    m_loadButton->setMinimumHeight(35);
    
    m_deleteButton = new QPushButton("🗑️ 删除", this);
    m_deleteButton->setEnabled(false);
    m_deleteButton->setMinimumHeight(35);
    
    m_closeButton = new QPushButton("关闭", this);
    m_closeButton->setMinimumHeight(35);
    
    buttonLayout->addWidget(m_loadButton);
    buttonLayout->addWidget(m_deleteButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_closeButton);
    
    mainLayout->addLayout(buttonLayout);
    
    // 连接信号
    connect(m_listWidget, &QListWidget::itemSelectionChanged,
            this, &ChatHistoryDialog::onItemSelectionChanged);
    connect(m_listWidget, &QListWidget::itemDoubleClicked,
            this, &ChatHistoryDialog::onItemDoubleClicked);
    connect(m_loadButton, &QPushButton::clicked,
            this, &ChatHistoryDialog::onLoadClicked);
    connect(m_deleteButton, &QPushButton::clicked,
            this, &ChatHistoryDialog::onDeleteClicked);
    connect(m_closeButton, &QPushButton::clicked,
            this, &QDialog::reject);
    
    // 加载对话列表
    loadConversationList();
}

void ChatHistoryDialog::loadConversationList()
{
    m_listWidget->clear();
    m_conversations.clear();
    
    QDir dir("data/conversations");
    if (!dir.exists()) {
        m_infoLabel->setText("暂无对话记录");
        return;
    }
    
    QStringList filters;
    filters << "*.json";
    QFileInfoList files = dir.entryInfoList(filters, QDir::Files, QDir::Time);
    
    for (const QFileInfo &fileInfo : files) {
        QFile file(fileInfo.filePath());
        if (!file.open(QIODevice::ReadOnly)) {
            continue;
        }
        
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        file.close();
        
        if (!doc.isObject()) {
            continue;
        }
        
        QJsonObject obj = doc.object();
        ConversationInfo info;
        info.questionId = obj["questionId"].toString();
        info.questionTitle = obj["questionTitle"].toString("未知题目");
        info.lastModified = fileInfo.lastModified();
        info.messageCount = obj["messages"].toArray().size();
        info.filePath = fileInfo.filePath();
        
        m_conversations.append(info);
        
        // 添加到列表
        QString itemText = QString("%1\n💬 %2 条消息 | 📅 %3")
            .arg(info.questionTitle)
            .arg(info.messageCount)
            .arg(formatDateTime(info.lastModified));
        
        QListWidgetItem *item = new QListWidgetItem(itemText);
        item->setData(Qt::UserRole, info.questionId);
        m_listWidget->addItem(item);
    }
    
    if (m_conversations.isEmpty()) {
        m_infoLabel->setText("暂无对话记录");
    } else {
        m_infoLabel->setText(QString("共 %1 条对话记录：").arg(m_conversations.size()));
    }
}

void ChatHistoryDialog::onLoadClicked()
{
    QListWidgetItem *item = m_listWidget->currentItem();
    if (!item) {
        return;
    }
    
    m_selectedId = item->data(Qt::UserRole).toString();
    emit conversationSelected(m_selectedId);
    accept();
}

void ChatHistoryDialog::onDeleteClicked()
{
    QListWidgetItem *item = m_listWidget->currentItem();
    if (!item) {
        return;
    }
    
    QString questionId = item->data(Qt::UserRole).toString();
    
    // 查找对话信息
    ConversationInfo *info = nullptr;
    for (ConversationInfo &conv : m_conversations) {
        if (conv.questionId == questionId) {
            info = &conv;
            break;
        }
    }
    
    if (!info) {
        return;
    }
    
    // 确认删除
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("确认删除");
    msgBox.setText(QString("确定要删除对话记录吗？\n\n题目：%1\n消息数：%2")
                   .arg(info->questionTitle)
                   .arg(info->messageCount));
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);
    
    if (msgBox.exec() == QMessageBox::Yes) {
        // 删除文件
        if (QFile::remove(info->filePath)) {
            emit conversationDeleted(questionId);
            
            // 重新加载列表
            loadConversationList();
            
            QMessageBox::information(this, "删除成功", "对话记录已删除");
        } else {
            QMessageBox::warning(this, "删除失败", "无法删除对话记录文件");
        }
    }
}

void ChatHistoryDialog::onItemSelectionChanged()
{
    updateButtonStates();
}

void ChatHistoryDialog::onItemDoubleClicked(QListWidgetItem *item)
{
    if (item) {
        onLoadClicked();
    }
}

void ChatHistoryDialog::updateButtonStates()
{
    bool hasSelection = m_listWidget->currentItem() != nullptr;
    m_loadButton->setEnabled(hasSelection);
    m_deleteButton->setEnabled(hasSelection);
}

QString ChatHistoryDialog::formatDateTime(const QDateTime &dt) const
{
    QDateTime now = QDateTime::currentDateTime();
    qint64 seconds = dt.secsTo(now);
    
    if (seconds < 60) {
        return "刚刚";
    } else if (seconds < 3600) {
        return QString("%1分钟前").arg(seconds / 60);
    } else if (seconds < 86400) {
        return QString("%1小时前").arg(seconds / 3600);
    } else if (seconds < 604800) {
        return QString("%1天前").arg(seconds / 86400);
    } else {
        return dt.toString("yyyy-MM-dd hh:mm");
    }
}
