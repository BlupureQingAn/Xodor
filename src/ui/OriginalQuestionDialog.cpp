#include "OriginalQuestionDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

OriginalQuestionDialog::OriginalQuestionDialog(const Question &question, QWidget *parent)
    : QDialog(parent)
    , m_question(question)
{
    setupUI();
    displayQuestion();
}

void OriginalQuestionDialog::setupUI()
{
    setWindowTitle("查看原题");
    resize(900, 700);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(16);
    
    // 标题
    QLabel *titleLabel = new QLabel(m_question.title(), this);
    titleLabel->setStyleSheet(
        "font-size: 18pt; "
        "font-weight: bold; "
        "color: #e8e8e8; "
        "padding: 10px;"
    );
    titleLabel->setWordWrap(true);
    
    // 难度标签
    QString diffText;
    QString diffColor;
    switch (m_question.difficulty()) {
        case Difficulty::Easy:
            diffText = "简单";
            diffColor = "#e8e8e8";
            break;
        case Difficulty::Medium:
            diffText = "中等";
            diffColor = "#b0b0b0";
            break;
        case Difficulty::Hard:
            diffText = "困难";
            diffColor = "#660000";
            break;
    }
    
    QLabel *diffLabel = new QLabel(QString("难度: %1").arg(diffText), this);
    diffLabel->setStyleSheet(QString(
        "font-size: 11pt; "
        "color: %1; "
        "font-weight: 600; "
        "padding: 5px 10px;"
    ).arg(diffColor));
    
    // 标签页
    QTabWidget *tabWidget = new QTabWidget(this);
    
    // 题目描述标签页
    m_descriptionBrowser = new QTextBrowser(this);
    m_descriptionBrowser->setOpenExternalLinks(false);
    tabWidget->addTab(m_descriptionBrowser, "📝 题目描述");
    
    // 测试用例标签页
    m_testCasesBrowser = new QTextBrowser(this);
    tabWidget->addTab(m_testCasesBrowser, "🧪 测试用例");
    
    // 参考答案标签页
    m_answerBrowser = new QTextBrowser(this);
    tabWidget->addTab(m_answerBrowser, "💡 参考答案");
    
    // 底部按钮
    QHBoxLayout *btnLayout = new QHBoxLayout();
    
    m_practiceBtn = new QPushButton("开始练习", this);
    m_closeBtn = new QPushButton("关闭", this);
    
    QString btnStyle = R"(
        QPushButton {
            background-color: #660000;
            color: white;
            border: none;
            border-radius: 10px;
            padding: 12px 24px;
            font-weight: 500;
            font-size: 11pt;
            min-width: 100px;
        }
        QPushButton:hover {
            background-color: #880000;
        }
        QPushButton:pressed {
            background-color: #440000;
        }
    )";
    
    m_practiceBtn->setStyleSheet(btnStyle);
    m_closeBtn->setStyleSheet(btnStyle);
    
    btnLayout->addStretch();
    btnLayout->addWidget(m_practiceBtn);
    btnLayout->addWidget(m_closeBtn);
    
    mainLayout->addWidget(titleLabel);
    mainLayout->addWidget(diffLabel);
    mainLayout->addWidget(tabWidget);
    mainLayout->addLayout(btnLayout);
    
    // 应用样式
    setStyleSheet(R"(
        QDialog {
            background-color: #242424;
        }
        QTextBrowser {
            background-color: #242424;
            color: #e8e8e8;
            border: 1px solid #3a3a3a;
            border-radius: 12px;
            padding: 16px;
            font-size: 10pt;
            line-height: 1.6;
        }
        QTabWidget::pane {
            border: 1px solid #3a3a3a;
            border-radius: 8px;
            background-color: #242424;
        }
        QTabBar::tab {
            background-color: #242424;
            color: #e8e8e8;
            border: 1px solid #3a3a3a;
            border-bottom: none;
            border-top-left-radius: 8px;
            border-top-right-radius: 8px;
            padding: 12px 24px;
            margin-right: 2px;
            font-weight: 500;
        }
        QTabBar::tab:selected {
            background-color: #660000;
        }
        QTabBar::tab:hover {
            background-color: #363636;
        }
    )");
    
    // 连接信号
    connect(m_practiceBtn, &QPushButton::clicked, this, [this]() {
        emit practiceRequested();
        accept();
    });
    connect(m_closeBtn, &QPushButton::clicked, this, &QDialog::reject);
}

void OriginalQuestionDialog::displayQuestion()
{
    // 显示题目描述
    QString descHtml = "<div style='line-height: 1.8;'>";
    descHtml += m_question.description().replace("\n", "<br>");
    
    // 添加标签
    if (!m_question.tags().isEmpty()) {
        descHtml += "<br><br><hr><p><b>标签：</b>";
        for (const QString &tag : m_question.tags()) {
            descHtml += QString("<span style='background-color: #242424; "
                              "padding: 4px 12px; border-radius: 6px; "
                              "margin-right: 8px; display: inline-block;'>%1</span>")
                .arg(tag);
        }
        descHtml += "</p>";
    }
    descHtml += "</div>";
    m_descriptionBrowser->setHtml(descHtml);
    
    // 显示测试用例
    QString testCasesHtml = "<div style='line-height: 1.8;'>";
    if (m_question.testCases().isEmpty()) {
        testCasesHtml += "<p style='color: #b0b0b0;'>暂无测试用例</p>";
    } else {
        testCasesHtml += "<h3>测试用例列表</h3>";
        int index = 1;
        for (const TestCase &tc : m_question.testCases()) {
            testCasesHtml += QString(
                "<div style='background-color: #242424; "
                "padding: 16px; margin: 12px 0; border-radius: 10px;'>"
                "<h4 style='color: #660000; margin-top: 0;'>测试用例 %1</h4>"
                "<p><b>输入：</b></p>"
                "<pre style='background-color: #242424; padding: 12px; "
                "border-radius: 6px; overflow-x: auto;'>%2</pre>"
                "<p><b>期望输出：</b></p>"
                "<pre style='background-color: #242424; padding: 12px; "
                "border-radius: 6px; overflow-x: auto;'>%3</pre>"
                "</div>"
            ).arg(index++).arg(tc.input.toHtmlEscaped()).arg(tc.expectedOutput.toHtmlEscaped());
        }
    }
    testCasesHtml += "</div>";
    m_testCasesBrowser->setHtml(testCasesHtml);
    
    // 显示参考答案
    QString answerHtml = "<div style='line-height: 1.8;'>";
    if (m_question.referenceAnswer().isEmpty()) {
        answerHtml += "<p style='color: #b0b0b0;'>暂无参考答案</p>";
    } else {
        answerHtml += "<h3>参考答案</h3>";
        answerHtml += "<pre style='background-color: #242424; "
                     "padding: 16px; border-radius: 10px; "
                     "overflow-x: auto; font-family: Consolas, Monaco, monospace;'>";
        answerHtml += m_question.referenceAnswer().toHtmlEscaped();
        answerHtml += "</pre>";
        
        answerHtml += "<br><p style='color: #b0b0b0;'>"
                     "💡 <b>提示：</b>参考答案仅供参考，鼓励你先独立思考和实现。</p>";
    }
    answerHtml += "</div>";
    m_answerBrowser->setHtml(answerHtml);
}
