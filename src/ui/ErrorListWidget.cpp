#include "ErrorListWidget.h"
#include "../ai/OllamaClient.h"
#include <QListWidgetItem>
#include <QIcon>
#include <QMetaType>
#include <QDebug>

// 注册自定义类型
Q_DECLARE_METATYPE(SyntaxError)

ErrorListWidget::ErrorListWidget(QWidget *parent)
    : QWidget(parent)
    , m_aiClient(nullptr)
{
    // 设置整体样式
    setStyleSheet(R"(
        QWidget {
            background-color: #1e1e1e;
            border-top: 1px solid #3a3a3a;
        }
    )");
    
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    
    // 顶部信息栏
    QHBoxLayout *topLayout = new QHBoxLayout();
    
    // 折叠/展开按钮
    QPushButton *toggleButton = new QPushButton("▼", this);
    toggleButton->setMaximumWidth(30);
    toggleButton->setMaximumHeight(28);
    toggleButton->setToolTip("折叠/展开错误列表");
    toggleButton->setStyleSheet(R"(
        QPushButton {
            background-color: #3a3a3a;
            border: 1px solid #555;
            border-radius: 3px;
            color: #ccc;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #4a4a4a;
        }
    )");
    
    m_countLabel = new QLabel("无错误", this);
    m_countLabel->setStyleSheet("color: #888; font-size: 11pt;");
    
    m_fixAllButton = new QPushButton("🔧 AI修复全部", this);
    m_fixAllButton->setEnabled(false);
    m_fixAllButton->setMaximumHeight(28);
    
    m_fixSelectedButton = new QPushButton("🔧 修复选中", this);
    m_fixSelectedButton->setEnabled(false);
    m_fixSelectedButton->setMaximumHeight(28);
    
    topLayout->addWidget(toggleButton);
    topLayout->addWidget(m_countLabel);
    topLayout->addStretch();
    topLayout->addWidget(m_fixSelectedButton);
    topLayout->addWidget(m_fixAllButton);
    
    mainLayout->addLayout(topLayout);
    
    // 连接折叠按钮
    connect(toggleButton, &QPushButton::clicked, this, [this, toggleButton]() {
        bool isVisible = m_listWidget->isVisible();
        m_listWidget->setVisible(!isVisible);
        toggleButton->setText(isVisible ? "▶" : "▼");
    });
    
    // 错误列表
    m_listWidget = new QListWidget(this);
    m_listWidget->setAlternatingRowColors(false);
    m_listWidget->setStyleSheet(R"(
        QListWidget {
            background-color: #1e1e1e;
            border: none;
            outline: none;
        }
        QListWidget::item {
            padding: 4px 8px;
            border-bottom: 1px solid #2a2a2a;
            color: #cccccc;
        }
        QListWidget::item:selected {
            background-color: #094771;
            color: white;
        }
        QListWidget::item:hover {
            background-color: #2a2d2e;
        }
    )");
    
    mainLayout->addWidget(m_listWidget);
    
    // 连接信号
    connect(m_listWidget, &QListWidget::itemClicked,
            this, &ErrorListWidget::onItemClicked);
    connect(m_fixAllButton, &QPushButton::clicked,
            this, &ErrorListWidget::onFixAllClicked);
    connect(m_fixSelectedButton, &QPushButton::clicked,
            this, &ErrorListWidget::onFixSelectedClicked);
}

void ErrorListWidget::setErrors(const QVector<SyntaxError> &errors)
{
    qDebug() << "[ErrorListWidget] setErrors called with" << errors.size() << "errors";
    
    m_errors = errors;
    m_listWidget->clear();
    
    for (const SyntaxError &error : errors) {
        QListWidgetItem *item = new QListWidgetItem();
        item->setText(formatErrorMessage(error));
        
        // 设置颜色
        if (error.type == "error") {
            item->setForeground(QColor("#ef4444"));
        } else {
            item->setForeground(QColor("#f59e0b"));
        }
        
        // 存储错误信息
        item->setData(Qt::UserRole, QVariant::fromValue(error));
        
        m_listWidget->addItem(item);
        
        qDebug() << "[ErrorListWidget] Added error:" << error.type 
                 << "at line" << error.line << "-" << error.message;
    }
    
    qDebug() << "[ErrorListWidget] Total items in list:" << m_listWidget->count();
    updateErrorCount();
}

void ErrorListWidget::onItemClicked(QListWidgetItem *item)
{
    if (!item) return;
    
    SyntaxError error = item->data(Qt::UserRole).value<SyntaxError>();
    emit errorClicked(error.line, error.column);
}

void ErrorListWidget::onFixAllClicked()
{
    if (m_errors.isEmpty() || !m_aiClient) {
        return;
    }
    
    emit fixAllRequested("", m_errors);
}

void ErrorListWidget::onFixSelectedClicked()
{
    QListWidgetItem *item = m_listWidget->currentItem();
    if (!item || !m_aiClient) {
        return;
    }
    
    SyntaxError error = item->data(Qt::UserRole).value<SyntaxError>();
    emit fixRequested("", error);
}

void ErrorListWidget::updateErrorCount()
{
    int errorCount = 0;
    int warningCount = 0;
    
    for (const SyntaxError &error : m_errors) {
        if (error.type == "error") {
            errorCount++;
        } else {
            warningCount++;
        }
    }
    
    if (errorCount == 0 && warningCount == 0) {
        m_countLabel->setText("✅ 无错误");
        m_countLabel->setStyleSheet("color: #10b981; font-size: 11pt;");
    } else {
        QString text = QString("❌ %1 个错误").arg(errorCount);
        if (warningCount > 0) {
            text += QString(", ⚠️ %1 个警告").arg(warningCount);
        }
        m_countLabel->setText(text);
        m_countLabel->setStyleSheet("color: #ef4444; font-size: 11pt;");
    }
    
    m_fixAllButton->setEnabled(!m_errors.isEmpty() && m_aiClient);
    m_fixSelectedButton->setEnabled(m_listWidget->currentItem() != nullptr && m_aiClient);
}

QString ErrorListWidget::formatErrorMessage(const SyntaxError &error) const
{
    return QString("第%1行:%2列 - %3")
        .arg(error.line)
        .arg(error.column)
        .arg(error.message);
}
