#include "QuestionPanel.h"
#include "../utils/MarkdownRenderer.h"
#include <QVBoxLayout>
#include <QHBoxLayout>

QuestionPanel::QuestionPanel(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
}

void QuestionPanel::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);
    
    m_questionBrowser = new ZoomableTextBrowser(this);
    m_questionBrowser->setOpenExternalLinks(false);
    
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(8);
    
    m_prevBtn = new QPushButton("◀ 上一题", this);
    m_aiJudgeBtn = new QPushButton("🤖 AI判题", this);
    m_aiJudgeBtn->setToolTip(QString::fromUtf8("让AI分析代码逻辑，判断是否符合题目要求"));
    m_nextBtn = new QPushButton("下一题 ▶", this);
    
    // 应用按钮样式
    QString buttonStyle = R"(
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
    )";
    
    m_prevBtn->setStyleSheet(buttonStyle);
    m_aiJudgeBtn->setStyleSheet(buttonStyle);
    m_nextBtn->setStyleSheet(buttonStyle);
    
    btnLayout->addWidget(m_prevBtn);
    btnLayout->addWidget(m_aiJudgeBtn);
    btnLayout->addWidget(m_nextBtn);
    
    mainLayout->addWidget(m_questionBrowser);
    mainLayout->addLayout(btnLayout);
    
    connect(m_aiJudgeBtn, &QPushButton::clicked, this, &QuestionPanel::aiJudgeRequested);
    connect(m_nextBtn, &QPushButton::clicked, this, &QuestionPanel::nextQuestion);
    connect(m_prevBtn, &QPushButton::clicked, this, &QuestionPanel::previousQuestion);
}

void QuestionPanel::setQuestion(const Question &question)
{
    // 构建现代化的HTML显示
    QString html = R"(
        <html>
        <head>
            <style>
                body {
                    font-family: 'Microsoft YaHei', 'Segoe UI', sans-serif;
                    line-height: 1.8;
                    color: #e8e8e8;
                    background-color: #242424;
                    padding: 20px;
                }
                h2 {
                    color: #e8e8e8;
                    border-bottom: 2px solid #660000;
                    padding-bottom: 10px;
                    margin-top: 0;
                }
                .meta {
                    background-color: #2d2d2d;
                    padding: 12px;
                    border-radius: 8px;
                    margin: 15px 0;
                }
                .difficulty {
                    display: inline-block;
                    padding: 4px 12px;
                    border-radius: 4px;
                    font-weight: bold;
                }
                .difficulty-easy { background-color: #2d5016; color: #a3d977; }
                .difficulty-medium { background-color: #5c4a1a; color: #ffc107; }
                .difficulty-hard { background-color: #660000; color: #ff6b6b; }
                .tags {
                    margin-top: 8px;
                }
                .tag {
                    display: inline-block;
                    background-color: #363636;
                    color: #b0b0b0;
                    padding: 4px 10px;
                    border-radius: 4px;
                    margin-right: 6px;
                    font-size: 9pt;
                }
                .content {
                    margin-top: 20px;
                }
                p {
                    margin: 6px 0;
                    white-space: pre-wrap;
                }
                code {
                    background-color: #2d2d2d;
                    color: #ff6b6b;
                    padding: 2px 6px;
                    border-radius: 3px;
                    font-family: 'Consolas', 'Monaco', monospace;
                    font-size: 9.5pt;
                }
                pre {
                    background-color: #1e1e1e;
                    border: 1px solid #3a3a3a;
                    border-radius: 6px;
                    padding: 15px;
                    overflow-x: auto;
                    margin: 15px 0;
                }
                pre code {
                    background-color: transparent;
                    color: #e8e8e8;
                    padding: 0;
                }
                .test-case {
                    background-color: #2d2d2d;
                    border-left: 3px solid #660000;
                    padding: 12px;
                    margin: 10px 0;
                    border-radius: 4px;
                }
                .test-case-title {
                    font-weight: bold;
                    color: #660000;
                    margin-bottom: 8px;
                }
                .io-block {
                    background-color: #1e1e1e;
                    padding: 12px;
                    border-radius: 4px;
                    margin: 8px 0;
                    font-family: 'Consolas', 'Monaco', monospace;
                    font-size: 10pt;
                    line-height: 1.6;
                    user-select: text;
                    white-space: pre-wrap;
                    word-wrap: break-word;
                    border: 1px solid #3a3a3a;
                    color: #e8e8e8;
                    max-height: 200px;
                    overflow-y: auto;
                    overflow-x: auto;
                }
                .io-block::-webkit-scrollbar {
                    width: 8px;
                    height: 8px;
                }
                .io-block::-webkit-scrollbar-track {
                    background: #2a2a2a;
                    border-radius: 4px;
                }
                .io-block::-webkit-scrollbar-thumb {
                    background: #555;
                    border-radius: 4px;
                }
                .io-block::-webkit-scrollbar-thumb:hover {
                    background: #666;
                }
                .io-label {
                    color: #b0b0b0;
                    font-weight: bold;
                    margin-bottom: 6px;
                    font-size: 10pt;
                }
            </style>
        </head>
        <body>
    )";
    
    // 标题
    html += QString("<h2>%1</h2>").arg(question.title().toHtmlEscaped());
    
    // 元信息
    html += "<div class='meta'>";
    
    // 难度
    QString diffText, diffClass;
    switch (question.difficulty()) {
        case Difficulty::Easy:
            diffText = "简单";
            diffClass = "difficulty-easy";
            break;
        case Difficulty::Medium:
            diffText = "中等";
            diffClass = "difficulty-medium";
            break;
        case Difficulty::Hard:
            diffText = "困难";
            diffClass = "difficulty-hard";
            break;
    }
    html += QString("<span class='difficulty %1'>%2</span>").arg(diffClass, diffText);
    
    // 标签
    if (!question.tags().isEmpty()) {
        html += "<div class='tags'>";
        for (const QString &tag : question.tags()) {
            html += QString("<span class='tag'>%1</span>").arg(tag.toHtmlEscaped());
        }
        html += "</div>";
    }
    
    html += "</div>";
    
    // 题目描述 - 使用统一的Markdown渲染器
    QString desc = MarkdownRenderer::toHtml(question.description(), true);
    html += "<div class='content'>" + desc + "</div>";
    
    // 测试用例
    if (!question.testCases().isEmpty()) {
        html += "<h3 style='color: #e8e8e8; margin-top: 25px;'>示例</h3>";
        html += "<p style='color: #b0b0b0; font-size: 9pt; margin: 8px 0;'>💡 提示：选中文本后按Ctrl+C复制</p>";
        int caseNum = 1;
        for (const TestCase &tc : question.testCases()) {
            html += QString("<div class='test-case'>");
            html += QString("<div class='test-case-title'>示例 %1</div>").arg(caseNum++);
            
            // 输入部分
            html += "<div class='io-label'>输入：</div>";
            QString inputHtml = tc.input.toHtmlEscaped();
            html += QString("<div class='io-block'>%1</div>").arg(inputHtml);
            
            // 输出部分
            html += "<div class='io-label'>输出：</div>";
            QString outputHtml = tc.expectedOutput.toHtmlEscaped();
            html += QString("<div class='io-block'>%1</div>").arg(outputHtml);
            
            html += "</div>";
        }
    }
    
    html += "</body></html>";
    
    m_questionBrowser->setHtml(html);
}
