#include "ExamReportDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>
#include <QPrinter>
#include <QPrintDialog>

ExamReportDialog::ExamReportDialog(const ExamReport &report, QWidget *parent)
    : QDialog(parent)
    , m_report(report)
{
    setupUI();
    displayReport();
    
    setWindowTitle(QString("%1 - 答题报告").arg(report.examName));
    resize(1000, 700);
}

void ExamReportDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(16);
    mainLayout->setContentsMargins(24, 24, 24, 24);
    
    // 标题
    QLabel *titleLabel = new QLabel(QString("📊 %1 - 答题报告").arg(m_report.examName), this);
    titleLabel->setStyleSheet("font-size: 18pt; font-weight: bold; color: #e8e8e8;");
    
    // 标签页
    m_tabWidget = new QTabWidget(this);
    
    m_summaryBrowser = new QTextBrowser(this);
    m_detailsBrowser = new QTextBrowser(this);
    m_analysisBrowser = new QTextBrowser(this);
    
    m_tabWidget->addTab(m_summaryBrowser, "📈 总览");
    m_tabWidget->addTab(m_detailsBrowser, "📝 详情");
    m_tabWidget->addTab(m_analysisBrowser, "🎓 分析");
    
    // 设置样式
    QString browserStyle = R"(
        QTextBrowser {
            background-color: #1a1a1a;
            color: #e8e8e8;
            border: 2px solid #3a3a3a;
            border-radius: 8px;
            padding: 15px;
            font-size: 10pt;
        }
    )";
    
    m_summaryBrowser->setStyleSheet(browserStyle);
    m_detailsBrowser->setStyleSheet(browserStyle);
    m_analysisBrowser->setStyleSheet(browserStyle);
    
    // 按钮
    QHBoxLayout *btnLayout = new QHBoxLayout();
    
    m_exportMdBtn = new QPushButton("导出Markdown", this);
    m_exportHtmlBtn = new QPushButton("导出HTML", this);
    m_exportJsonBtn = new QPushButton("导出JSON", this);
    m_printBtn = new QPushButton("打印", this);
    m_closeBtn = new QPushButton("关闭", this);
    
    connect(m_exportMdBtn, &QPushButton::clicked, this, &ExamReportDialog::onExportMarkdown);
    connect(m_exportHtmlBtn, &QPushButton::clicked, this, &ExamReportDialog::onExportHtml);
    connect(m_exportJsonBtn, &QPushButton::clicked, this, &ExamReportDialog::onExportJson);
    connect(m_printBtn, &QPushButton::clicked, this, &ExamReportDialog::onPrint);
    connect(m_closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    
    btnLayout->addWidget(m_exportMdBtn);
    btnLayout->addWidget(m_exportHtmlBtn);
    btnLayout->addWidget(m_exportJsonBtn);
    btnLayout->addWidget(m_printBtn);
    btnLayout->addStretch();
    btnLayout->addWidget(m_closeBtn);
    
    // 布局
    mainLayout->addWidget(titleLabel);
    mainLayout->addWidget(m_tabWidget, 1);
    mainLayout->addLayout(btnLayout);
    
    // 样式
    QString btnStyle = R"(
        QPushButton {
            background-color: #660000;
            color: white;
            border: none;
            border-radius: 8px;
            padding: 10px 20px;
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
    )";
    
    m_exportMdBtn->setStyleSheet(btnStyle);
    m_exportHtmlBtn->setStyleSheet(btnStyle);
    m_exportJsonBtn->setStyleSheet(btnStyle);
    m_printBtn->setStyleSheet(btnStyle);
    m_closeBtn->setStyleSheet(btnStyle);
    
    setStyleSheet(R"(
        QDialog {
            background-color: #242424;
        }
        QLabel {
            color: #e8e8e8;
        }
        QTabWidget::pane {
            border: 2px solid #3a3a3a;
            border-radius: 8px;
            background-color: #1e1e1e;
        }
        QTabBar::tab {
            background-color: #2a2a2a;
            color: #e8e8e8;
            padding: 10px 20px;
            border: 2px solid #3a3a3a;
            border-bottom: none;
            border-top-left-radius: 8px;
            border-top-right-radius: 8px;
            margin-right: 2px;
        }
        QTabBar::tab:selected {
            background-color: #660000;
        }
        QTabBar::tab:hover {
            background-color: #3a3a3a;
        }
    )");
}

void ExamReportDialog::displayReport()
{
    displaySummary();
    displayDetails();
    displayAnalysis();
}

void ExamReportDialog::displaySummary()
{
    QString html = "<html><body style='color: #e8e8e8; font-family: Arial;'>";
    
    // 基本信息
    html += "<h2 style='color: #ff6666;'>📊 基本信息</h2>";
    html += "<table style='width: 100%; border-collapse: collapse;'>";
    html += QString("<tr><td style='padding: 8px;'><b>考试名称：</b></td><td style='padding: 8px;'>%1</td></tr>").arg(m_report.examName);
    html += QString("<tr><td style='padding: 8px;'><b>分类：</b></td><td style='padding: 8px;'>%1</td></tr>").arg(m_report.category);
    html += QString("<tr><td style='padding: 8px;'><b>开始时间：</b></td><td style='padding: 8px;'>%1</td></tr>")
        .arg(m_report.startTime.toString("yyyy-MM-dd hh:mm:ss"));
    html += QString("<tr><td style='padding: 8px;'><b>结束时间：</b></td><td style='padding: 8px;'>%1</td></tr>")
        .arg(m_report.endTime.toString("yyyy-MM-dd hh:mm:ss"));
    html += QString("<tr><td style='padding: 8px;'><b>用时：</b></td><td style='padding: 8px;'>%1 / %2 分钟</td></tr>")
        .arg(m_report.actualTimeSpent).arg(m_report.totalTimeLimit);
    html += "</table>";
    
    // 成绩统计
    html += "<h2 style='color: #ff6666; margin-top: 30px;'>🎯 成绩统计</h2>";
    html += "<div style='display: flex; flex-wrap: wrap;'>";
    html += QString("<div style='background: #2a2a2a; padding: 20px; margin: 10px; border-radius: 10px; min-width: 150px;'>"
                   "<div style='font-size: 32pt; font-weight: bold; color: #66ff66;'>%1</div>"
                   "<div style='color: #aaa;'>总题数</div></div>").arg(m_report.totalQuestions);
    html += QString("<div style='background: #2a2a2a; padding: 20px; margin: 10px; border-radius: 10px; min-width: 150px;'>"
                   "<div style='font-size: 32pt; font-weight: bold; color: #66ff66;'>%1</div>"
                   "<div style='color: #aaa;'>正确数</div></div>").arg(m_report.correctQuestions);
    html += QString("<div style='background: #2a2a2a; padding: 20px; margin: 10px; border-radius: 10px; min-width: 150px;'>"
                   "<div style='font-size: 32pt; font-weight: bold; color: #ffaa00;'>%.1f%%</div>"
                   "<div style='color: #aaa;'>正确率</div></div>").arg(m_report.overallAccuracy);
    html += QString("<div style='background: #2a2a2a; padding: 20px; margin: 10px; border-radius: 10px; min-width: 150px;'>"
                   "<div style='font-size: 32pt; font-weight: bold; color: #ff6666;'>%1</div>"
                   "<div style='color: #aaa;'>总得分</div></div>").arg(m_report.totalScore);
    html += "</div>";
    
    // 建议
    if (!m_report.suggestions.isEmpty()) {
        html += "<h2 style='color: #ff6666; margin-top: 30px;'>💡 建议</h2>";
        html += "<div style='background: #2a2a2a; padding: 20px; border-radius: 10px; white-space: pre-wrap;'>";
        html += m_report.suggestions.toHtmlEscaped().replace("\n", "<br>");
        html += "</div>";
    }
    
    html += "</body></html>";
    
    m_summaryBrowser->setHtml(html);
}

void ExamReportDialog::displayDetails()
{
    QString html = "<html><body style='color: #e8e8e8; font-family: Arial;'>";
    
    html += "<h2 style='color: #ff6666;'>📝 答题详情</h2>";
    
    for (int i = 0; i < m_report.attempts.size(); ++i) {
        const QuestionAttempt &attempt = m_report.attempts[i];
        
        QString statusColor = attempt.isCorrect ? "#66ff66" : "#ff6666";
        QString statusIcon = attempt.isCorrect ? "✅" : "❌";
        
        html += QString("<div style='background: #2a2a2a; padding: 20px; margin: 15px 0; border-radius: 10px; border-left: 5px solid %1;'>")
            .arg(statusColor);
        html += QString("<h3 style='margin-top: 0;'>%1 %2. %3</h3>")
            .arg(statusIcon).arg(i + 1).arg(attempt.questionTitle);
        
        QString diffName;
        switch (attempt.difficulty) {
            case Difficulty::Easy: diffName = "简单"; break;
            case Difficulty::Medium: diffName = "中等"; break;
            case Difficulty::Hard: diffName = "困难"; break;
        }
        
        html += QString("<p><b>难度：</b>%1 &nbsp;&nbsp; <b>知识点：</b>%2</p>")
            .arg(diffName).arg(attempt.tags.join(", "));
        html += QString("<p><b>用时：</b>%1秒 &nbsp;&nbsp; <b>测试通过：</b>%2/%3 (%.1f%%)</p>")
            .arg(attempt.timeSpent)
            .arg(attempt.passedTestCases)
            .arg(attempt.totalTestCases)
            .arg(attempt.passRate());
        
        if (!attempt.isCorrect && !attempt.errorMessage.isEmpty()) {
            html += QString("<p style='color: #ff6666;'><b>错误信息：</b>%1</p>")
                .arg(attempt.errorMessage.toHtmlEscaped());
        }
        
        html += "</div>";
    }
    
    html += "</body></html>";
    
    m_detailsBrowser->setHtml(html);
}

void ExamReportDialog::displayAnalysis()
{
    QString html = "<html><body style='color: #e8e8e8; font-family: Arial;'>";
    
    // 难度分析
    html += "<h2 style='color: #ff6666;'>📈 难度分析</h2>";
    html += "<table style='width: 100%; border-collapse: collapse;'>";
    html += "<tr style='background: #660000;'>"
           "<th style='padding: 12px; text-align: left;'>难度</th>"
           "<th style='padding: 12px; text-align: center;'>题目数</th>"
           "<th style='padding: 12px; text-align: center;'>正确数</th>"
           "<th style='padding: 12px; text-align: center;'>正确率</th>"
           "<th style='padding: 12px; text-align: center;'>平均用时</th></tr>";
    
    QList<Difficulty> diffs = {Difficulty::Easy, Difficulty::Medium, Difficulty::Hard};
    for (Difficulty diff : diffs) {
        if (m_report.difficultyStats.contains(diff)) {
            const DifficultyStatistics &stats = m_report.difficultyStats[diff];
            QString diffName;
            switch (diff) {
                case Difficulty::Easy: diffName = "简单"; break;
                case Difficulty::Medium: diffName = "中等"; break;
                case Difficulty::Hard: diffName = "困难"; break;
            }
            html += QString("<tr style='border-bottom: 1px solid #3a3a3a;'>"
                           "<td style='padding: 12px;'>%1</td>"
                           "<td style='padding: 12px; text-align: center;'>%2</td>"
                           "<td style='padding: 12px; text-align: center;'>%3</td>"
                           "<td style='padding: 12px; text-align: center;'>%.1f%%</td>"
                           "<td style='padding: 12px; text-align: center;'>%4秒</td></tr>")
                .arg(diffName)
                .arg(stats.totalQuestions)
                .arg(stats.correctQuestions)
                .arg(stats.accuracy)
                .arg(stats.avgTimeSpent);
        }
    }
    html += "</table>";
    
    // 知识点分析
    html += "<h2 style='color: #ff6666; margin-top: 30px;'>🎓 知识点分析</h2>";
    html += "<table style='width: 100%; border-collapse: collapse;'>";
    html += "<tr style='background: #660000;'>"
           "<th style='padding: 12px; text-align: left;'>知识点</th>"
           "<th style='padding: 12px; text-align: center;'>题目数</th>"
           "<th style='padding: 12px; text-align: center;'>正确数</th>"
           "<th style='padding: 12px; text-align: center;'>正确率</th></tr>";
    
    QList<QString> topics = m_report.topicStats.keys();
    std::sort(topics.begin(), topics.end(), [this](const QString &a, const QString &b) {
        return m_report.topicStats[a].accuracy < m_report.topicStats[b].accuracy;
    });
    
    for (const QString &topic : topics) {
        const TopicStatistics &stats = m_report.topicStats[topic];
        QString rowColor = stats.accuracy < 60.0 ? "background: #3a2020;" : "";
        html += QString("<tr style='border-bottom: 1px solid #3a3a3a; %1'>"
                       "<td style='padding: 12px;'>%2</td>"
                       "<td style='padding: 12px; text-align: center;'>%3</td>"
                       "<td style='padding: 12px; text-align: center;'>%4</td>"
                       "<td style='padding: 12px; text-align: center;'>%.1f%%</td></tr>")
            .arg(rowColor)
            .arg(topic)
            .arg(stats.totalQuestions)
            .arg(stats.correctQuestions)
            .arg(stats.accuracy);
    }
    html += "</table>";
    
    // 薄弱知识点
    if (!m_report.weakTopics.isEmpty()) {
        html += "<h2 style='color: #ff6666; margin-top: 30px;'>⚠️ 薄弱知识点</h2>";
        for (const QString &topic : m_report.weakTopics) {
            const TopicStatistics &stats = m_report.topicStats[topic];
            html += QString("<div style='background: #3a2020; padding: 15px; margin: 10px 0; border-radius: 8px; border-left: 4px solid #ff6666;'>");
            html += QString("<h3 style='margin: 0 0 10px 0;'>%1 <span style='color: #ff6666;'>(%.1f%%)</span></h3>")
                .arg(topic).arg(stats.accuracy);
            if (!stats.weakQuestions.isEmpty()) {
                html += "<p style='margin: 5px 0;'><b>需要加强的题目：</b></p><ul style='margin: 5px 0;'>";
                for (const QString &q : stats.weakQuestions) {
                    html += QString("<li>%1</li>").arg(q);
                }
                html += "</ul>";
            }
            html += "</div>";
        }
    }
    
    html += "</body></html>";
    
    m_analysisBrowser->setHtml(html);
}

void ExamReportDialog::onExportMarkdown()
{
    QString fileName = QFileDialog::getSaveFileName(
        this,
        "导出Markdown报告",
        QString("%1_报告.md").arg(m_report.examName),
        "Markdown Files (*.md)"
    );
    
    if (fileName.isEmpty()) {
        return;
    }
    
    ExamReportGenerator generator;
    if (generator.saveReportAsMarkdown(m_report, fileName)) {
        QMessageBox::information(this, "导出成功", 
            QString("报告已导出到：\n%1").arg(fileName));
    } else {
        QMessageBox::critical(this, "导出失败", "无法保存报告文件。");
    }
}

void ExamReportDialog::onExportHtml()
{
    QString fileName = QFileDialog::getSaveFileName(
        this,
        "导出HTML报告",
        QString("%1_报告.html").arg(m_report.examName),
        "HTML Files (*.html)"
    );
    
    if (fileName.isEmpty()) {
        return;
    }
    
    ExamReportGenerator generator;
    if (generator.saveReportAsHtml(m_report, fileName)) {
        QMessageBox::information(this, "导出成功", 
            QString("报告已导出到：\n%1").arg(fileName));
    } else {
        QMessageBox::critical(this, "导出失败", "无法保存报告文件。");
    }
}

void ExamReportDialog::onExportJson()
{
    QString fileName = QFileDialog::getSaveFileName(
        this,
        "导出JSON报告",
        QString("%1_报告.json").arg(m_report.examName),
        "JSON Files (*.json)"
    );
    
    if (fileName.isEmpty()) {
        return;
    }
    
    ExamReportGenerator generator;
    if (generator.saveReport(m_report, fileName)) {
        QMessageBox::information(this, "导出成功", 
            QString("报告已导出到：\n%1").arg(fileName));
    } else {
        QMessageBox::critical(this, "导出失败", "无法保存报告文件。");
    }
}

void ExamReportDialog::onPrint()
{
    QPrinter printer;
    QPrintDialog dialog(&printer, this);
    
    if (dialog.exec() == QDialog::Accepted) {
        m_summaryBrowser->print(&printer);
    }
}
