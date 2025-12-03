#include "CodeVersionDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QSplitter>

CodeVersionDialog::CodeVersionDialog(const QString &questionId, const QString &questionTitle,
                                   CodeVersionManager *versionManager, QWidget *parent)
    : QDialog(parent)
    , m_versionManager(versionManager)
    , m_questionId(questionId)
    , m_questionTitle(questionTitle)
{
    setupUI();
    loadVersions();
    
    setWindowTitle("代码版本历史");
    resize(900, 600);
}

void CodeVersionDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(16);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    
    // 标题
    m_titleLabel = new QLabel(QString("📜 代码版本历史 - %1").arg(m_questionTitle), this);
    m_titleLabel->setStyleSheet("font-size: 16pt; font-weight: bold; color: #e8e8e8;");
    
    // 版本数量
    m_countLabel = new QLabel("", this);
    m_countLabel->setStyleSheet("color: #888; font-size: 10pt;");
    
    // 分割器
    QSplitter *splitter = new QSplitter(Qt::Horizontal, this);
    
    // 左侧：版本列表
    QWidget *leftWidget = new QWidget(this);
    QVBoxLayout *leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    
    QLabel *listLabel = new QLabel("版本列表：", this);
    listLabel->setStyleSheet("color: #e8e8e8; font-weight: bold;");
    
    m_versionList = new QListWidget(this);
    m_versionList->setStyleSheet(R"(
        QListWidget {
            background-color: #1e1e1e;
            color: #e8e8e8;
            border: 2px solid #3a3a3a;
            border-radius: 8px;
            padding: 8px;
            font-family: 'Consolas', 'Monaco', monospace;
            font-size: 9pt;
        }
        QListWidget::item {
            padding: 8px;
            border-radius: 4px;
            margin: 2px;
        }
        QListWidget::item:selected {
            background-color: #660000;
        }
        QListWidget::item:hover {
            background-color: #2a2a2a;
        }
    )");
    
    leftLayout->addWidget(listLabel);
    leftLayout->addWidget(m_versionList);
    
    // 右侧：代码预览
    QWidget *rightWidget = new QWidget(this);
    QVBoxLayout *rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    
    QLabel *previewLabel = new QLabel("代码预览：", this);
    previewLabel->setStyleSheet("color: #e8e8e8; font-weight: bold;");
    
    m_codePreview = new QTextEdit(this);
    m_codePreview->setReadOnly(true);
    m_codePreview->setStyleSheet(R"(
        QTextEdit {
            background-color: #1a1a1a;
            color: #e8e8e8;
            border: 2px solid #3a3a3a;
            border-radius: 8px;
            padding: 12px;
            font-family: 'Consolas', 'Monaco', monospace;
            font-size: 10pt;
            line-height: 1.4;
        }
    )");
    
    rightLayout->addWidget(previewLabel);
    rightLayout->addWidget(m_codePreview);
    
    splitter->addWidget(leftWidget);
    splitter->addWidget(rightWidget);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 2);
    
    // 按钮
    QHBoxLayout *btnLayout = new QHBoxLayout();
    
    m_restoreBtn = new QPushButton("恢复此版本", this);
    m_deleteBtn = new QPushButton("删除", this);
    m_refreshBtn = new QPushButton("刷新", this);
    m_closeBtn = new QPushButton("关闭", this);
    
    m_restoreBtn->setEnabled(false);
    m_deleteBtn->setEnabled(false);
    
    QString btnStyle = R"(
        QPushButton {
            background-color: #660000;
            color: white;
            border: none;
            border-radius: 8px;
            padding: 10px 20px;
            font-weight: 600;
            font-size: 10pt;
            min-width: 90px;
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
    
    m_restoreBtn->setStyleSheet(btnStyle);
    m_deleteBtn->setStyleSheet(btnStyle);
    m_refreshBtn->setStyleSheet(btnStyle);
    m_closeBtn->setStyleSheet(btnStyle);
    
    btnLayout->addWidget(m_restoreBtn);
    btnLayout->addWidget(m_deleteBtn);
    btnLayout->addStretch();
    btnLayout->addWidget(m_refreshBtn);
    btnLayout->addWidget(m_closeBtn);
    
    // 添加到主布局
    mainLayout->addWidget(m_titleLabel);
    mainLayout->addWidget(m_countLabel);
    mainLayout->addWidget(splitter, 1);
    mainLayout->addLayout(btnLayout);
    
    // 连接信号
    connect(m_versionList, &QListWidget::itemClicked,
            this, &CodeVersionDialog::onVersionSelected);
    connect(m_restoreBtn, &QPushButton::clicked,
            this, &CodeVersionDialog::onRestoreClicked);
    connect(m_deleteBtn, &QPushButton::clicked,
            this, &CodeVersionDialog::onDeleteClicked);
    connect(m_refreshBtn, &QPushButton::clicked,
            this, &CodeVersionDialog::onRefreshClicked);
    connect(m_closeBtn, &QPushButton::clicked,
            this, &QDialog::accept);
}

void CodeVersionDialog::loadVersions()
{
    m_versions = m_versionManager->getVersions(m_questionId);
    
    m_versionList->clear();
    m_codePreview->clear();
    m_selectedVersionId.clear();
    
    if (m_versions.isEmpty()) {
        m_countLabel->setText("暂无代码版本");
        m_codePreview->setPlainText("暂无代码版本\n\n提示：编写代码并保存后，会自动创建版本记录。");
        return;
    }
    
    m_countLabel->setText(QString("共 %1 个版本").arg(m_versions.size()));
    
    for (const CodeVersion &version : m_versions) {
        QString itemText = formatVersionItem(version);
        QListWidgetItem *item = new QListWidgetItem(itemText);
        item->setData(Qt::UserRole, version.versionId);
        m_versionList->addItem(item);
    }
    
    // 默认选中第一个（最新版本）
    if (m_versionList->count() > 0) {
        m_versionList->setCurrentRow(0);
        onVersionSelected(m_versionList->item(0));
    }
}

QString CodeVersionDialog::formatVersionItem(const CodeVersion &version) const
{
    QString icon = getStatusIcon(version.testPassed);
    QString timeStr = version.timestamp.toString("yyyy-MM-dd HH:mm:ss");
    QString testStr = version.testResult.isEmpty() ? "未测试" : version.testResult;
    
    return QString("%1 %2  (%3 行)  [%4]")
        .arg(icon)
        .arg(timeStr)
        .arg(version.lineCount)
        .arg(testStr);
}

QString CodeVersionDialog::getStatusIcon(bool testPassed) const
{
    return testPassed ? "✅" : "❌";
}

void CodeVersionDialog::onVersionSelected(QListWidgetItem *item)
{
    if (!item) return;
    
    m_selectedVersionId = item->data(Qt::UserRole).toString();
    
    // 查找对应的版本
    for (const CodeVersion &version : m_versions) {
        if (version.versionId == m_selectedVersionId) {
            m_codePreview->setPlainText(version.code);
            m_restoreBtn->setEnabled(true);
            m_deleteBtn->setEnabled(true);
            break;
        }
    }
}

void CodeVersionDialog::onRestoreClicked()
{
    if (m_selectedVersionId.isEmpty()) {
        return;
    }
    
    // 查找选中的版本
    for (const CodeVersion &version : m_versions) {
        if (version.versionId == m_selectedVersionId) {
            int ret = QMessageBox::question(this, "确认恢复",
                QString("确定要恢复到此版本吗？\n\n时间：%1\n行数：%2\n测试结果：%3\n\n当前代码将被覆盖！")
                    .arg(version.timestamp.toString("yyyy-MM-dd HH:mm:ss"))
                    .arg(version.lineCount)
                    .arg(version.testResult.isEmpty() ? "未测试" : version.testResult),
                QMessageBox::Yes | QMessageBox::No);
            
            if (ret == QMessageBox::Yes) {
                emit versionRestored(version.code);
                QMessageBox::information(this, "恢复成功", "代码已恢复到选中的版本！");
                accept();
            }
            break;
        }
    }
}

void CodeVersionDialog::onDeleteClicked()
{
    if (m_selectedVersionId.isEmpty()) {
        return;
    }
    
    int ret = QMessageBox::question(this, "确认删除",
        "确定要删除此版本吗？\n\n此操作不可恢复！",
        QMessageBox::Yes | QMessageBox::No);
    
    if (ret == QMessageBox::Yes) {
        if (m_versionManager->deleteVersion(m_questionId, m_selectedVersionId)) {
            QMessageBox::information(this, "删除成功", "版本已删除！");
            loadVersions();  // 刷新列表
        } else {
            QMessageBox::warning(this, "删除失败", "无法删除此版本！");
        }
    }
}

void CodeVersionDialog::onRefreshClicked()
{
    loadVersions();
}

QString CodeVersionDialog::getSelectedVersionCode() const
{
    if (m_selectedVersionId.isEmpty()) {
        return QString();
    }
    
    for (const CodeVersion &version : m_versions) {
        if (version.versionId == m_selectedVersionId) {
            return version.code;
        }
    }
    
    return QString();
}
