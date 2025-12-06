#include "ErrorListWidget.h"
#include "../ai/OllamaClient.h"
#include <QListWidgetItem>
#include <QIcon>
#include <QMetaType>
#include <QDebug>
#include <QMenu>
#include <QClipboard>
#include <QApplication>
#include <QToolTip>
#include <QMap>

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
    
    // 标题图标
    QLabel *titleIcon = new QLabel("🐛", this);
    titleIcon->setStyleSheet("font-size: 14pt;");
    
    m_countLabel = new QLabel("无错误", this);
    m_countLabel->setStyleSheet("color: #888; font-size: 11pt; font-weight: bold;");
    
    m_fixAllButton = new QPushButton("🔧 AI修复全部", this);
    m_fixAllButton->setEnabled(false);
    m_fixAllButton->setMaximumHeight(28);
    m_fixAllButton->setStyleSheet(R"(
        QPushButton {
            background-color: #3a3a3a;
            border: 1px solid #555;
            border-radius: 4px;
            color: #ccc;
            padding: 4px 12px;
        }
        QPushButton:hover {
            background-color: #4a4a4a;
        }
        QPushButton:disabled {
            background-color: #2a2a2a;
            color: #666;
        }
    )");
    
    m_fixSelectedButton = new QPushButton("🔧 修复选中", this);
    m_fixSelectedButton->setEnabled(false);
    m_fixSelectedButton->setMaximumHeight(28);
    m_fixSelectedButton->setStyleSheet(m_fixAllButton->styleSheet());
    
    // 关闭按钮
    // 不提供关闭按钮，错误列表应始终可见（无错误时自动隐藏）
    
    topLayout->addWidget(titleIcon);
    topLayout->addWidget(m_countLabel);
    topLayout->addStretch();
    topLayout->addWidget(m_fixSelectedButton);
    topLayout->addWidget(m_fixAllButton);
    
    mainLayout->addLayout(topLayout);
    
    // 错误列表
    m_listWidget = new QListWidget(this);
    m_listWidget->setAlternatingRowColors(false);
    m_listWidget->setContextMenuPolicy(Qt::CustomContextMenu);  // 启用右键菜单
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
            outline: none;
        }
        QListWidget::item:selected {
            background-color: #660000;
            color: white;
            outline: none;
        }
        QListWidget::item:selected:hover {
            background-color: #880000;
            outline: none;
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
    connect(m_listWidget, &QListWidget::customContextMenuRequested,
            this, &ErrorListWidget::onContextMenu);
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
    QString translatedMessage = translateErrorMessage(error.message);
    return QString("第%1行:%2列 - %3")
        .arg(error.line)
        .arg(error.column)
        .arg(translatedMessage);
}

QString ErrorListWidget::translateErrorMessage(const QString &message) const
{
    // 常见编译错误的中文翻译
    static QMap<QString, QString> translations = {
        {"expected initializer before", "期望在...之前有初始化器"},
        {"expected ';' before", "期望在...之前有分号 ';'"},
        {"expected ',' or ';' before", "期望在...之前有逗号 ',' 或分号 ';'"},
        {"expected unqualified-id before", "期望在...之前有标识符"},
        {"expected '(' before", "期望在...之前有左括号 '('"},
        {"expected ')' before", "期望在...之前有右括号 ')'"},
        {"expected '{' before", "期望在...之前有左大括号 '{'"},
        {"expected '}' before", "期望在...之前有右大括号 '}'"},
        {"expected '[' before", "期望在...之前有左方括号 '['"},
        {"expected ']' before", "期望在...之前有右方括号 ']'"},
        {"expected declaration before", "期望在...之前有声明"},
        {"expected primary-expression before", "期望在...之前有主表达式"},
        {"was not declared in this scope", "未在此作用域中声明"},
        {"undeclared identifier", "未声明的标识符"},
        {"redefinition of", "重复定义"},
        {"conflicting declaration", "冲突的声明"},
        {"invalid use of", "无效使用"},
        {"cannot convert", "无法转换"},
        {"no matching function", "没有匹配的函数"},
        {"too few arguments", "参数太少"},
        {"too many arguments", "参数太多"},
        {"invalid operands", "无效的操作数"},
        {"does not name a type", "不是一个类型名"},
        {"incomplete type", "不完整的类型"},
        {"'return'", "'return' 返回语句"}
    };
    
    QString result = message;
    
    // 尝试匹配并替换
    for (auto it = translations.constBegin(); it != translations.constEnd(); ++it) {
        if (message.contains(it.key(), Qt::CaseInsensitive)) {
            result.replace(it.key(), it.value(), Qt::CaseInsensitive);
        }
    }
    
    return result;
}

void ErrorListWidget::onContextMenu(const QPoint &pos)
{
    QListWidgetItem *item = m_listWidget->itemAt(pos);
    if (!item) {
        return;
    }
    
    QMenu menu(this);
    menu.setStyleSheet(R"(
        QMenu {
            background-color: #2d2d2d;
            border: 1px solid #555;
            padding: 4px;
        }
        QMenu::item {
            padding: 6px 20px;
            color: #e0e0e0;
        }
        QMenu::item:selected {
            background-color: #3d3d3d;
        }
    )");
    
    QAction *copyAction = menu.addAction("📋 复制错误信息");
    QAction *copyAllAction = menu.addAction("📋 复制所有错误");
    menu.addSeparator();
    QAction *jumpAction = menu.addAction("🔍 跳转到错误位置");
    
    QAction *selectedAction = menu.exec(m_listWidget->mapToGlobal(pos));
    
    if (selectedAction == copyAction) {
        // 复制当前错误
        QClipboard *clipboard = QApplication::clipboard();
        clipboard->setText(item->text());
        
        QToolTip::showText(m_listWidget->mapToGlobal(pos), 
                          "✅ 已复制错误信息", m_listWidget, QRect(), 1500);
    }
    else if (selectedAction == copyAllAction) {
        // 复制所有错误
        QStringList allErrors;
        for (int i = 0; i < m_listWidget->count(); ++i) {
            allErrors.append(m_listWidget->item(i)->text());
        }
        
        QClipboard *clipboard = QApplication::clipboard();
        clipboard->setText(allErrors.join("\n"));
        
        QToolTip::showText(m_listWidget->mapToGlobal(pos), 
                          QString("✅ 已复制 %1 条错误信息").arg(allErrors.size()), 
                          m_listWidget, QRect(), 1500);
    }
    else if (selectedAction == jumpAction) {
        // 跳转到错误位置
        onItemClicked(item);
    }
}
