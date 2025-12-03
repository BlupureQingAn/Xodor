#include "QuestionPanel.h"
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
    
    m_questionBrowser = new QTextBrowser(this);
    m_questionBrowser->setOpenExternalLinks(false);
    
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(8);
    
    m_prevBtn = new QPushButton("⬅ 上一题", this);
    m_runTestsBtn = new QPushButton("▶ 运行测试", this);
    m_nextBtn = new QPushButton("下一题 ➡", this);
    
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
    m_runTestsBtn->setStyleSheet(buttonStyle);
    m_nextBtn->setStyleSheet(buttonStyle);
    
    btnLayout->addWidget(m_prevBtn);
    btnLayout->addWidget(m_runTestsBtn);
    btnLayout->addWidget(m_nextBtn);
    
    mainLayout->addWidget(m_questionBrowser);
    mainLayout->addLayout(btnLayout);
    
    connect(m_runTestsBtn, &QPushButton::clicked, this, &QuestionPanel::runTests);
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
                    margin: 12px 0;
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
                    padding: 10px;
                    border-radius: 4px;
                    margin: 6px 0;
                    font-family: 'Consolas', 'Monaco', monospace;
                    font-size: 9.5pt;
                    user-select: text;
                }
                .io-label {
                    color: #b0b0b0;
                    font-weight: bold;
                    margin-bottom: 4px;
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
    
    // 题目描述 - 转换Markdown格式
    QString desc = question.description();
    desc = convertMarkdownToHtml(desc);
    html += "<div class='content'>" + desc + "</div>";
    
    // 测试用例
    if (!question.testCases().isEmpty()) {
        html += "<h3 style='color: #e8e8e8; margin-top: 25px;'>示例</h3>";
        int caseNum = 1;
        for (const TestCase &tc : question.testCases()) {
            html += QString("<div class='test-case'>");
            html += QString("<div class='test-case-title'>示例 %1</div>").arg(caseNum++);
            html += "<div class='io-label'>输入：</div>";
            html += QString("<div class='io-block'>%1</div>").arg(tc.input.toHtmlEscaped());
            html += "<div class='io-label'>输出：</div>";
            html += QString("<div class='io-block'>%1</div>").arg(tc.expectedOutput.toHtmlEscaped());
            html += "</div>";
        }
    }
    
    html += "</body></html>";
    
    m_questionBrowser->setHtml(html);
}

QString QuestionPanel::convertMarkdownToHtml(const QString &markdown)
{
    QString html = markdown;
    
    // 处理代码块
    QRegularExpression codeBlockRegex(R"(```(\w*)\n([\s\S]*?)```)", QRegularExpression::MultilineOption);
    html.replace(codeBlockRegex, "<pre><code>\\2</code></pre>");
    
    // 处理行内代码
    QRegularExpression inlineCodeRegex(R"(`([^`]+)`)");
    html.replace(inlineCodeRegex, "<code>\\1</code>");
    
    // 处理数学公式 $...$
    QRegularExpression mathInlineRegex(R"(\$([^\$]+)\$)");
    html.replace(mathInlineRegex, "<span style='color: #ffc107; font-style: italic;'>\\1</span>");
    
    // 处理数学公式 $$...$$
    QRegularExpression mathBlockRegex(R"(\$\$([\s\S]+?)\$\$)", QRegularExpression::MultilineOption);
    html.replace(mathBlockRegex, "<div style='text-align: center; color: #ffc107; font-style: italic; margin: 15px 0;'>\\1</div>");
    
    // 处理LaTeX命令
    html.replace(R"(\frac)", "frac");
    html.replace(R"(\partial)", "∂");
    html.replace(R"(\cdot)", "·");
    html.replace(R"(\dots)", "...");
    html.replace(R"(\le)", "≤");
    html.replace(R"(\ge)", "≥");
    html.replace(R"(\times)", "×");
    html.replace(R"(\div)", "÷");
    html.replace(R"(\sum)", "Σ");
    html.replace(R"(\prod)", "Π");
    html.replace(R"(\int)", "∫");
    html.replace(R"(\sqrt)", "√");
    html.replace(R"(\alpha)", "α");
    html.replace(R"(\beta)", "β");
    html.replace(R"(\gamma)", "γ");
    html.replace(R"(\delta)", "δ");
    html.replace(R"(\pi)", "π");
    html.replace(R"(\theta)", "θ");
    html.replace(R"(\lambda)", "λ");
    html.replace(R"(\mu)", "μ");
    html.replace(R"(\sigma)", "σ");
    html.replace(R"(\infty)", "∞");
    html.replace(R"(\in)", "∈");
    html.replace(R"(\subset)", "⊂");
    html.replace(R"(\cup)", "∪");
    html.replace(R"(\cap)", "∩");
    html.replace(R"(\emptyset)", "∅");
    html.replace(R"(\forall)", "∀");
    html.replace(R"(\exists)", "∃");
    html.replace(R"(\neg)", "¬");
    html.replace(R"(\land)", "∧");
    html.replace(R"(\lor)", "∨");
    html.replace(R"(\rightarrow)", "→");
    html.replace(R"(\leftarrow)", "←");
    html.replace(R"(\Rightarrow)", "⇒");
    html.replace(R"(\Leftarrow)", "⇐");
    
    // 处理粗体
    QRegularExpression boldRegex(R"(\*\*([^\*]+)\*\*)");
    html.replace(boldRegex, "<b>\\1</b>");
    
    // 处理斜体
    QRegularExpression italicRegex(R"(\*([^\*]+)\*)");
    html.replace(italicRegex, "<i>\\1</i>");
    
    // 处理换行
    html.replace("\n\n", "</p><p>");
    html.replace("\n", "<br>");
    
    // 转义HTML特殊字符（但保留我们添加的标签）
    // html = html.toHtmlEscaped(); // 不能全部转义，会破坏我们的HTML标签
    
    return html;
}
