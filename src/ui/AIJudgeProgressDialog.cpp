#include "AIJudgeProgressDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

AIJudgeProgressDialog::AIJudgeProgressDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("AI判题中");
    setModal(true);
    setFixedSize(350, 150);
    
    // 移除窗口边框按钮
    setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(20);
    mainLayout->setContentsMargins(30, 30, 30, 30);
    
    // 图标和消息
    QHBoxLayout *topLayout = new QHBoxLayout();
    topLayout->setSpacing(15);
    
    m_iconLabel = new QLabel(this);
    m_iconLabel->setText("🤖");
    m_iconLabel->setStyleSheet("font-size: 32pt;");
    m_iconLabel->setFixedSize(50, 50);
    m_iconLabel->setAlignment(Qt::AlignCenter);
    
    m_messageLabel = new QLabel("正在分析代码...", this);
    m_messageLabel->setStyleSheet("font-size: 11pt; color: #e8e8e8;");
    m_messageLabel->setWordWrap(true);
    
    topLayout->addWidget(m_iconLabel);
    topLayout->addWidget(m_messageLabel, 1);
    
    mainLayout->addLayout(topLayout);
    
    // 进度条（居中）
    QHBoxLayout *progressLayout = new QHBoxLayout();
    progressLayout->addStretch();
    
    m_progressBar = new RedProgressBar(this);
    progressLayout->addWidget(m_progressBar);
    
    progressLayout->addStretch();
    
    mainLayout->addLayout(progressLayout);
    mainLayout->addStretch();
    
    // 设置样式
    setStyleSheet(R"(
        QDialog {
            background-color: #1e1e1e;
        }
    )");
}

void AIJudgeProgressDialog::setMessage(const QString &message)
{
    m_messageLabel->setText(message);
}
