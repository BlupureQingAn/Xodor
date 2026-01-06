#include "SmartImportDialog.h"
#include "../ai/OllamaClient.h"
#include "../utils/ConfigManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTimer>
#include <QDebug>
#include <cmath>

SmartImportDialog::SmartImportDialog(const QString &sourcePath, const QString &bankName,
                                   OllamaClient *aiClient, QWidget *parent)
    : QDialog(parent)
    , m_sourcePath(sourcePath)
    , m_bankName(bankName)
    , m_success(false)
{
    // 设置目标路径
    m_targetPath = QString("data/question_banks/%1").arg(bankName);
    
    // 重新加载AI配置（确保使用最新配置）
    ConfigManager &config = ConfigManager::instance();
    if (aiClient) {
        if (config.useCloudApi()) {
            aiClient->setCloudMode(true);
            aiClient->setBaseUrl(config.cloudApiUrl());
            aiClient->setModel(config.cloudApiModel());
            aiClient->setApiKey(config.cloudApiKey());
            qDebug() << "[SmartImportDialog] 使用云端API:" << config.cloudApiUrl() << "模型:" << config.cloudApiModel();
        } else {
            aiClient->setCloudMode(false);
            aiClient->setBaseUrl(config.ollamaUrl());
            aiClient->setModel(config.ollamaModel());
            qDebug() << "[SmartImportDialog] 使用本地Ollama:" << config.ollamaUrl() << "模型:" << config.ollamaModel();
        }
    } else {
        qWarning() << "[SmartImportDialog] AI客户端为空！";
    }
    
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
    m_logText->clear();
    m_logText->append("=== AI智能导入开始 ===\n");
    m_logText->append(QString("📚 题库: %1").arg(m_bankName));
    m_logText->append(QString("📁 源路径: %1\n").arg(m_sourcePath));
    
    m_logText->append("💡 AI将自动识别格式、解析题目、生成测试数据并实时保存\n");
    
    // 使用AI解析器（完整的AI驱动流程）
    m_statusLabel->setText("准备扫描文件...");
    m_importer->startImport(m_sourcePath, m_targetPath, m_bankName);
}

void SmartImportDialog::onProgressUpdated(const ImportProgress &progress)
{
    // 更新状态
    m_statusLabel->setText(progress.currentStatus);
    
    // 更新统计信息
    QString stats;
    switch (progress.currentStage) {
        case ImportProgress::Scanning:
            stats = QString("扫描: %1/%2 个文件")
                .arg(progress.processedFiles)
                .arg(progress.totalFiles);
            break;
        case ImportProgress::Parsing:
            stats = QString("已识别并保存: %1 道题目")
                .arg(progress.totalQuestions);
            break;
        case ImportProgress::Saving:
            stats = QString("保存中... %1 道题目")
                .arg(progress.totalQuestions);
            break;
        case ImportProgress::Complete:
            stats = QString("完成！共 %1 道题目")
                .arg(progress.totalQuestions);
            break;
        default:
            stats = "准备中...";
            break;
    }
    m_statsLabel->setText(stats);
    
    // 使用新的进度计算方法
    int percentage = progress.calculatePercentage();
    m_progressBar->setValue(percentage);
    m_progressBar->setFormat(QString("%1%").arg(percentage));
}

void SmartImportDialog::onLogMessage(const QString &message)
{
    m_logText->append(message);
    
    // 自动滚动到底部
    QTextCursor cursor = m_logText->textCursor();
    cursor.movePosition(QTextCursor::End);
    m_logText->setTextCursor(cursor);
}

void SmartImportDialog::onImportCompleted(const ImportResult &result)
{
    m_success = result.success;
    
    if (result.success) {
        m_statusLabel->setText("✅ 导入完成！");
        m_progressBar->setValue(100);
        
        // 构建详细的完成消息
        QString completionMsg = QString("\n🎉 导入完成！共导入 %1 道题目\n").arg(result.totalQuestions);
        completionMsg += QString("📁 保存位置：%1\n").arg(result.basePath);
        
        // 按源文件统计
        if (!result.questionsByFile.isEmpty()) {
            completionMsg += "\n📄 按源文件分类：\n";
            for (auto it = result.questionsByFile.constBegin(); it != result.questionsByFile.constEnd(); ++it) {
                completionMsg += QString("  • %1: %2 道题目\n").arg(it.key()).arg(it.value());
            }
        }
        
        // 按难度统计
        if (!result.questionsByDifficulty.isEmpty()) {
            completionMsg += "\n📊 按难度分类：\n";
            int total = result.totalQuestions;
            for (auto it = result.questionsByDifficulty.constBegin(); it != result.questionsByDifficulty.constEnd(); ++it) {
                QString emoji;
                if (it.key() == "简单") emoji = "🟢";
                else if (it.key() == "中等") emoji = "🟡";
                else if (it.key() == "困难") emoji = "🔴";
                else emoji = "⚪";
                
                double percentage = total > 0 ? (it.value() * 100.0 / total) : 0;
                completionMsg += QString("  %1 %2: %3 道题目 (%4%)\n")
                    .arg(emoji).arg(it.key()).arg(it.value()).arg(percentage, 0, 'f', 1);
            }
        }
        
        completionMsg += "\n💡 提示：现在可以在题库面板中查看和练习这些题目了！";
        
        m_logText->append(completionMsg);
    } else {
        m_statusLabel->setText("❌ 导入失败");
        
        QString errorMsg = QString("\n❌ 导入失败：%1").arg(result.errorMessage);
        
        // 如果有部分导入的题目，显示统计
        if (result.totalQuestions > 0) {
            errorMsg += QString("\n\n⚠️ 已导入 %1 道题目（部分成功）").arg(result.totalQuestions);
            errorMsg += QString("\n📁 保存位置：%1").arg(result.basePath);
        }
        
        // 显示警告信息
        if (!result.warnings.isEmpty()) {
            errorMsg += "\n\n⚠️ 警告信息：\n";
            for (const QString &warning : result.warnings) {
                errorMsg += QString("  • %1\n").arg(warning);
            }
        }
        
        m_logText->append(errorMsg);
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
