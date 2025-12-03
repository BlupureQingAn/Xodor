#include "AIAnalysisPanel.h"
#include <QVBoxLayout>

AIAnalysisPanel::AIAnalysisPanel(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
}

void AIAnalysisPanel::setupUI()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(8);
    
    m_analyzeBtn = new QPushButton("🤖 AI分析代码", this);
    m_analysisBrowser = new QTextBrowser(this);
    
    // 应用按钮样式
    m_analyzeBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #660000;
            color: white;
            border: none;
            border-radius: 6px;
            padding: 10px 20px;
            font-weight: 500;
            font-size: 10pt;
        }
        QPushButton:hover {
            background-color: #880000;
        }
        QPushButton:pressed {
            background-color: #440000;
        }
    )");
    
    layout->addWidget(m_analyzeBtn);
    layout->addWidget(m_analysisBrowser);
    
    connect(m_analyzeBtn, &QPushButton::clicked, 
            this, &AIAnalysisPanel::requestAnalysis);
}

void AIAnalysisPanel::setAnalysis(const QString &analysis)
{
    m_analysisBrowser->setHtml(analysis);
}

void AIAnalysisPanel::setVisible(bool visible)
{
    QWidget::setVisible(visible);
}
