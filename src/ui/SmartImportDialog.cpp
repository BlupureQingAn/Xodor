#include "SmartImportDialog.h"
#include "../ai/OllamaClient.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTimer>

SmartImportDialog::SmartImportDialog(const QString &sourcePath, const QString &bankName,
                                   OllamaClient *aiClient, QWidget *parent)
    : QDialog(parent)
    , m_sourcePath(sourcePath)
    , m_bankName(bankName)
    , m_success(false)
{
    // 设置目标路径
    m_targetPath = QString("data/question_banks/%1").arg(bankName);
    
    // 创建导入器
    m_importer = new SmartQuestionImporter(aiClient, this);
    
    setupUI();
    setWindowTitle("智能导入题库");
    resize(800, 600);
    
    // 连接信号
    connect(m_importer, &SmartQuestionImporter::progressUpdated,
            this, &SmartImportDialog::onProgressUpdated);
    connect(m_importer, &SmartQuestionImporter::logMessage,
            this, &SmartImportDialog::onLogMessage);
    connect(m_importer, &SmartQuestionImporter::importCompleted,
            this, &SmartImportDialog::onImportCompleted);
    
    // 延迟启动
    QTimer::singleShot(500, this, &SmartImportDialog::startImport);
}

void SmartImportDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(16);
    mainLayout->setContentsMargins(24, 24, 24, 24);
    
    // 标题
    m_titleLabel = new QLabel("🤖 AI智能导入题库", this);
    m_titleLabel->setStyleSheet("font-size: 18pt; font-weight: bold; color: #e8e8e8;");
    
    // 说明文本
    QLabel *infoLabel = new QLabel(
        "💡 AI将自动识别题目格式、解析题目、生成测试数据\n"
        "   全程自动化，无需手动配置",
        this
    );
    infoLabel->setStyleSheet("color: #b0b0b0; font-size: 10pt; padding: 10px; background: #1a1a1a; border-radius: 8px;");
    infoLabel->setWordWrap(true);
    
    // 状态标签
    m_statusLabel = new QLabel("准备开始...", this);
    m_statusLabel->setStyleSheet("color: #b0b0b0; font-size: 11pt;");
    
    // 统计信息
    m_statsLabel = new QLabel("", this);
    m_statsLabel->setStyleSheet("color: #888; font-size: 10pt;");
    
    // 进度条
    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setTextVisible(true);
    m_progressBar->setStyleSheet(R"(
        QProgressBar {
            border: 2px solid #3a3a3a;
            border-radius: 10px;
            background-color: #1e1e1e;
            text-align: center;
            color: #e8e8e8;
            height: 32px;
            font-weight: bold;
        }
        QProgressBar::chunk {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 #660000, stop:1 #aa0000);
            border-radius: 8px;
        }
    )");
    
    // 日志区域
    QLabel *logLabel = new QLabel("📋 处理日志:", this);
    logLabel->setStyleSheet("color: #e8e8e8; font-weight: bold; font-size: 10pt;");
    
    m_logText = new QTextEdit(this);
    m_logText->setReadOnly(true);
    m_logText->setStyleSheet(R"(
        QTextEdit {
            background-color: #1a1a1a;
            color: #e8e8e8;
            border: 2px solid #3a3a3a;
            border-radius: 10px;
            padding: 12px;
            font-family: 'Consolas', 'Monaco', monospace;
            font-size: 9pt;
            line-height: 1.5;
        }
    )");
    
    // 按钮
    QHBoxLayout *btnLayout = new QHBoxLayout();
    m_cancelBtn = new QPushButton("取消", this);
    m_closeBtn = new QPushButton("完成", this);
    m_closeBtn->setEnabled(false);
    
    QString btnStyle = R"(
        QPushButton {
            background-color: #660000;
            color: white;
            border: none;
            border-radius: 10px;
            padding: 12px 28px;
            font-weight: 600;
            font-size: 10pt;
            min-width: 100px;
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
    
    m_cancelBtn->setStyleSheet(btnStyle);
    m_closeBtn->setStyleSheet(btnStyle);
    
    connect(m_cancelBtn, &QPushButton::clicked, this, &SmartImportDialog::onCancelClicked);
    connect(m_closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    
    btnLayout->addStretch();
    btnLayout->addWidget(m_cancelBtn);
    btnLayout->addWidget(m_closeBtn);
    
    // 布局
    mainLayout->addWidget(m_titleLabel);
    mainLayout->addWidget(infoLabel);
    mainLayout->addWidget(m_statusLabel);
    mainLayout->addWidget(m_statsLabel);
    mainLayout->addWidget(m_progressBar);
    mainLayout->addSpacing(8);
    mainLayout->addWidget(logLabel);
    mainLayout->addWidget(m_logText, 1);
    mainLayout->addLayout(btnLayout);
    
    // 对话框样式
    setStyleSheet(R"(
        QDialog {
            background-color: #242424;
        }
        QLabel {
            color: #e8e8e8;
        }
    )");
}

void SmartImportDialog::startImport()
{
    m_logText->append(QString("📚 题库名称: %1").arg(m_bankName));
    m_logText->append(QString("📁 源路径: %1").arg(m_sourcePath));
    m_logText->append(QString("🎯 目标路径: %1\n").arg(m_targetPath));
    
    // 统一使用AI智能解析
    m_logText->append("🤖 使用AI智能解析模式\n");
    m_logText->append("📋 AI将自动识别格式、解析题目、生成测试数据\n\n");
    
    // 使用通用解析器（包含完整的保存流程）
    m_statusLabel->setText("🤖 AI智能解析中...");
    m_importer->startImportWithUniversalParser(m_sourcePath, m_targetPath, m_bankName);
}

void SmartImportDialog::onProgressUpdated(const ImportProgress &progress)
{
    // 更新状态
    m_statusLabel->setText(progress.currentStatus);
    
    // 更新统计信息
    QString stats = QString("已处理: %1/%2 文件块 | 已导入: %3 道题目")
        .arg(progress.processedChunks)
        .arg(progress.totalChunks)
        .arg(progress.totalQuestions);
    m_statsLabel->setText(stats);
    
    // 更新进度条
    if (progress.totalChunks > 0) {
        int percentage = (progress.processedChunks * 100) / progress.totalChunks;
        m_progressBar->setValue(percentage);
    }
}

void SmartImportDialog::onLogMessage(const QString &message)
{
    m_logText->append(message);
    
    // 自动滚动到底部
    QTextCursor cursor = m_logText->textCursor();
    cursor.movePosition(QTextCursor::End);
    m_logText->setTextCursor(cursor);
}

void SmartImportDialog::onImportCompleted(bool success, const QString &message)
{
    m_success = success;
    
    if (success) {
        m_statusLabel->setText("✅ 导入完成！");
        m_progressBar->setValue(100);
        m_logText->append(QString("\n🎉 %1").arg(message));
    } else {
        m_statusLabel->setText("❌ 导入失败");
        m_logText->append(QString("\n❌ %1").arg(message));
    }
    
    m_cancelBtn->setEnabled(false);
    m_closeBtn->setEnabled(true);
}

void SmartImportDialog::onCancelClicked()
{
    m_importer->cancelImport();
    m_cancelBtn->setEnabled(false);
}

QVector<Question> SmartImportDialog::getImportedQuestions() const
{
    return m_importer->getImportedQuestions();
}


// getSelectedMode函数已移除，统一使用AI解析
