#include "ExamReportGenerator.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>

ExamReportGenerator::ExamReportGenerator()
{
}

ExamReport ExamReportGenerator::generateReport(const ExamSession &session)
{
    ExamReport report;
    
    // 基本信息
    report.sessionId = session.sessionId();
    report.examName = session.examName();
    report.category = session.category();
    report.startTime = session.startTime();
    report.endTime = session.endTime();
    
    // 时间统计
    report.totalTimeLimit = session.totalTimeLimit();
    report.actualTimeSpent = session.timeSpent();
    report.isTimeout = session.isTimeout();
    
    // 题目统计
    report.totalQuestions = session.totalQuestions();
    report.attemptedQuestions = session.attempts().size();
    report.correctQuestions = session.correctCount();
    report.overallAccuracy = session.accuracy();
    report.totalScore = session.totalScore();
    
    // 答题详情
    report.attempts = session.attempts();
    
    // 分析知识点
    analyzeTopics(session, report);
    
    // 分析难度
    analyzeDifficulty(session, report);
    
    // 识别薄弱知识点
    identifyWeakTopics(report);
    
    // 生成建议
    report.suggestions = generateSuggestions(report);
    
    return report;
}

void ExamReportGenerator::analyzeTopics(const ExamSession &session, ExamReport &report)
{
    QMap<QString, QVector<const QuestionAttempt*>> topicAttempts;
    
    // 按知识点分组
    for (const QuestionAttempt &attempt : session.attempts()) {
        for (const QString &tag : attempt.tags) {
            topicAttempts[tag].append(&attempt);
        }
    }
    
    // 统计每个知识点
    for (auto it = topicAttempts.begin(); it != topicAttempts.end(); ++it) {
        TopicStatistics stats;
        stats.topicName = it.key();
        stats.totalQuestions = it.value().size();
        stats.correctQuestions = 0;
        
        for (const QuestionAttempt *attempt : it.value()) {
            if (attempt->isCorrect) {
                stats.correctQuestions++;
            } else {
                stats.weakQuestions.append(attempt->questionTitle);
            }
        }
        
        stats.accuracy = stats.totalQuestions > 0 
            ? (double)stats.correctQuestions / stats.totalQuestions * 100.0 
            : 0.0;
        
        report.topicStats[it.key()] = stats;
    }
}

void ExamReportGenerator::analyzeDifficulty(const ExamSession &session, ExamReport &report)
{
    QMap<Difficulty, QVector<const QuestionAttempt*>> diffAttempts;
    
    // 按难度分组
    for (const QuestionAttempt &attempt : session.attempts()) {
        diffAttempts[attempt.difficulty].append(&attempt);
    }
    
    // 统计每个难度
    for (auto it = diffAttempts.begin(); it != diffAttempts.end(); ++it) {
        DifficultyStatistics stats;
        stats.difficulty = it.key();
        stats.totalQuestions = it.value().size();
        stats.correctQuestions = 0;
        
        int totalTime = 0;
        for (const QuestionAttempt *attempt : it.value()) {
            if (attempt->isCorrect) {
                stats.correctQuestions++;
            }
            totalTime += attempt->timeSpent;
        }
        
        stats.accuracy = stats.totalQuestions > 0 
            ? (double)stats.correctQuestions / stats.totalQuestions * 100.0 
            : 0.0;
        
        stats.avgTimeSpent = stats.totalQuestions > 0 
            ? totalTime / stats.totalQuestions 
            : 0;
        
        report.difficultyStats[it.key()] = stats;
    }
}

void ExamReportGenerator::identifyWeakTopics(ExamReport &report)
{
    // 找出正确率低于60%的知识点
    for (auto it = report.topicStats.begin(); it != report.topicStats.end(); ++it) {
        if (it.value().accuracy < 60.0) {
            report.weakTopics.append(it.key());
        }
    }
    
    // 按正确率排序（从低到高）
    std::sort(report.weakTopics.begin(), report.weakTopics.end(),
              [&report](const QString &a, const QString &b) {
        return report.topicStats[a].accuracy < report.topicStats[b].accuracy;
    });
}

QString ExamReportGenerator::generateSuggestions(const ExamReport &report)
{
    QString suggestions;
    
    // 总体评价
    if (report.overallAccuracy >= 90.0) {
        suggestions += "🎉 优秀！你的整体表现非常出色。\n\n";
    } else if (report.overallAccuracy >= 70.0) {
        suggestions += "👍 良好！你的整体表现不错，继续保持。\n\n";
    } else if (report.overallAccuracy >= 60.0) {
        suggestions += "💪 及格！还有提升空间，加油！\n\n";
    } else {
        suggestions += "📚 需要加强！建议多加练习。\n\n";
    }
    
    // 时间管理
    if (report.isTimeout) {
        suggestions += "⏰ 时间管理：本次答题超时，建议提高答题速度。\n\n";
    } else if (report.actualTimeSpent < report.totalTimeLimit * 0.7) {
        suggestions += "⏱️ 时间管理：答题速度较快，可以多花时间检查代码。\n\n";
    }
    
    // 薄弱知识点
    if (!report.weakTopics.isEmpty()) {
        suggestions += "📖 薄弱知识点：\n";
        for (int i = 0; i < qMin(3, report.weakTopics.size()); ++i) {
            const QString &topic = report.weakTopics[i];
            const TopicStatistics &stats = report.topicStats[topic];
            suggestions += QString("  • %1 (正确率: %.1f%%)\n")
                .arg(topic)
                .arg(stats.accuracy);
        }
        suggestions += "\n建议针对这些知识点进行专项练习。\n\n";
    }
    
    // 难度建议
    for (auto it = report.difficultyStats.begin(); it != report.difficultyStats.end(); ++it) {
        if (it.value().accuracy < 50.0) {
            suggestions += QString("🎯 %1题目：正确率较低(%.1f%%)，建议加强基础训练。\n\n")
                .arg(formatDifficulty(it.key()))
                .arg(it.value().accuracy);
        }
    }
    
    return suggestions;
}

QString ExamReportGenerator::formatTime(int seconds) const
{
    int hours = seconds / 3600;
    int minutes = (seconds % 3600) / 60;
    int secs = seconds % 60;
    
    if (hours > 0) {
        return QString("%1小时%2分%3秒").arg(hours).arg(minutes).arg(secs);
    } else if (minutes > 0) {
        return QString("%1分%2秒").arg(minutes).arg(secs);
    } else {
        return QString("%1秒").arg(secs);
    }
}

QString ExamReportGenerator::formatDifficulty(Difficulty diff) const
{
    switch (diff) {
        case Difficulty::Easy: return "简单";
        case Difficulty::Medium: return "中等";
        case Difficulty::Hard: return "困难";
        default: return "未知";
    }
}

QString ExamReport::toMarkdown() const
{
    QString md;
    
    // 标题
    md += QString("# %1 - 答题报告\n\n").arg(examName);
    
    // 基本信息
    md += "## 📊 基本信息\n\n";
    md += QString("- **考试名称**：%1\n").arg(examName);
    md += QString("- **分类**：%1\n").arg(category);
    md += QString("- **开始时间**：%1\n").arg(startTime.toString("yyyy-MM-dd hh:mm:ss"));
    md += QString("- **结束时间**：%1\n").arg(endTime.toString("yyyy-MM-dd hh:mm:ss"));
    md += QString("- **时间限制**：%1 分钟\n").arg(totalTimeLimit);
    md += QString("- **实际用时**：%1 分钟\n").arg(actualTimeSpent);
    if (isTimeout) {
        md += "- **状态**：⏰ 超时\n";
    }
    md += "\n";
    
    // 成绩统计
    md += "## 🎯 成绩统计\n\n";
    md += QString("- **总题数**：%1 道\n").arg(totalQuestions);
    md += QString("- **已答题数**：%1 道\n").arg(attemptedQuestions);
    md += QString("- **正确题数**：%1 道\n").arg(correctQuestions);
    md += QString("- **正确率**：%.1f%%\n").arg(overallAccuracy);
    md += QString("- **总得分**：%1 分\n").arg(totalScore);
    md += "\n";
    
    // 难度分析
    md += "## 📈 难度分析\n\n";
    md += "| 难度 | 题目数 | 正确数 | 正确率 | 平均用时 |\n";
    md += "|------|--------|--------|--------|----------|\n";
    
    QList<Difficulty> diffs = {Difficulty::Easy, Difficulty::Medium, Difficulty::Hard};
    for (Difficulty diff : diffs) {
        if (difficultyStats.contains(diff)) {
            const DifficultyStatistics &stats = difficultyStats[diff];
            QString diffName;
            switch (diff) {
                case Difficulty::Easy: diffName = "简单"; break;
                case Difficulty::Medium: diffName = "中等"; break;
                case Difficulty::Hard: diffName = "困难"; break;
            }
            md += QString("| %1 | %2 | %3 | %.1f%% | %4秒 |\n")
                .arg(diffName)
                .arg(stats.totalQuestions)
                .arg(stats.correctQuestions)
                .arg(stats.accuracy)
                .arg(stats.avgTimeSpent);
        }
    }
    md += "\n";
    
    // 知识点分析
    md += "## 🎓 知识点分析\n\n";
    md += "| 知识点 | 题目数 | 正确数 | 正确率 |\n";
    md += "|--------|--------|--------|--------|\n";
    
    // 按正确率排序
    QList<QString> topics = topicStats.keys();
    std::sort(topics.begin(), topics.end(), [this](const QString &a, const QString &b) {
        return topicStats[a].accuracy < topicStats[b].accuracy;
    });
    
    for (const QString &topic : topics) {
        const TopicStatistics &stats = topicStats[topic];
        md += QString("| %1 | %2 | %3 | %.1f%% |\n")
            .arg(topic)
            .arg(stats.totalQuestions)
            .arg(stats.correctQuestions)
            .arg(stats.accuracy);
    }
    md += "\n";
    
    // 薄弱知识点
    if (!weakTopics.isEmpty()) {
        md += "## ⚠️ 薄弱知识点\n\n";
        for (const QString &topic : weakTopics) {
            const TopicStatistics &stats = topicStats[topic];
            md += QString("### %1 (正确率: %.1f%%)\n\n").arg(topic).arg(stats.accuracy);
            if (!stats.weakQuestions.isEmpty()) {
                md += "需要加强的题目：\n";
                for (const QString &q : stats.weakQuestions) {
                    md += QString("- %1\n").arg(q);
                }
                md += "\n";
            }
        }
    }
    
    // 答题详情
    md += "## 📝 答题详情\n\n";
    for (int i = 0; i < attempts.size(); ++i) {
        const QuestionAttempt &attempt = attempts[i];
        md += QString("### %1. %2\n\n").arg(i + 1).arg(attempt.questionTitle);
        
        QString diffName;
        switch (attempt.difficulty) {
            case Difficulty::Easy: diffName = "简单"; break;
            case Difficulty::Medium: diffName = "中等"; break;
            case Difficulty::Hard: diffName = "困难"; break;
        }
        
        md += QString("- **难度**：%1\n").arg(diffName);
        md += QString("- **知识点**：%1\n").arg(attempt.tags.join(", "));
        md += QString("- **用时**：%1秒\n").arg(attempt.timeSpent);
        md += QString("- **测试通过**：%1/%2\n").arg(attempt.passedTestCases).arg(attempt.totalTestCases);
        md += QString("- **结果**：%1\n").arg(attempt.isCorrect ? "✅ 通过" : "❌ 未通过");
        
        if (!attempt.isCorrect && !attempt.errorMessage.isEmpty()) {
            md += QString("- **错误信息**：%1\n").arg(attempt.errorMessage);
        }
        md += "\n";
    }
    
    // 建议
    if (!suggestions.isEmpty()) {
        md += "## 💡 建议\n\n";
        md += suggestions;
    }
    
    md += "\n---\n\n";
    md += QString("*报告生成时间：%1*\n").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
    
    return md;
}

bool ExamReportGenerator::saveReport(const ExamReport &report, const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    
    QJsonDocument doc(report.toJson());
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    return true;
}

bool ExamReportGenerator::saveReportAsMarkdown(const ExamReport &report, const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    
    file.write(report.toMarkdown().toUtf8());
    file.close();
    return true;
}

bool ExamReportGenerator::saveReportAsHtml(const ExamReport &report, const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    
    file.write(report.toHtml().toUtf8());
    file.close();
    return true;
}

QString ExamReport::toHtml() const
{
    QString html;
    
    html += "<!DOCTYPE html>\n";
    html += "<html>\n<head>\n";
    html += "<meta charset=\"UTF-8\">\n";
    html += QString("<title>%1 - 答题报告</title>\n").arg(examName);
    html += "<style>\n";
    html += "body { font-family: Arial, sans-serif; margin: 20px; background: #f5f5f5; }\n";
    html += ".container { max-width: 1000px; margin: 0 auto; background: white; padding: 30px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }\n";
    html += "h1 { color: #660000; border-bottom: 3px solid #660000; padding-bottom: 10px; }\n";
    html += "h2 { color: #880000; margin-top: 30px; }\n";
    html += "table { width: 100%; border-collapse: collapse; margin: 20px 0; }\n";
    html += "th, td { padding: 12px; text-align: left; border-bottom: 1px solid #ddd; }\n";
    html += "th { background-color: #660000; color: white; }\n";
    html += "tr:hover { background-color: #f5f5f5; }\n";
    html += ".stat-box { display: inline-block; margin: 10px; padding: 15px 25px; background: #f0f0f0; border-radius: 8px; }\n";
    html += ".correct { color: #28a745; font-weight: bold; }\n";
    html += ".incorrect { color: #dc3545; font-weight: bold; }\n";
    html += ".weak-topic { background: #fff3cd; padding: 15px; margin: 10px 0; border-left: 4px solid #ffc107; border-radius: 4px; }\n";
    html += ".suggestion { background: #d1ecf1; padding: 15px; margin: 10px 0; border-left: 4px solid #17a2b8; border-radius: 4px; }\n";
    html += "</style>\n";
    html += "</head>\n<body>\n";
    html += "<div class=\"container\">\n";
    
    // 标题
    html += QString("<h1>%1 - 答题报告</h1>\n").arg(examName);
    
    // 基本信息
    html += "<h2>📊 基本信息</h2>\n";
    html += "<div class=\"stat-box\"><strong>考试名称：</strong>" + examName + "</div>\n";
    html += "<div class=\"stat-box\"><strong>分类：</strong>" + category + "</div>\n";
    html += "<div class=\"stat-box\"><strong>用时：</strong>" + QString::number(actualTimeSpent) + "/" + QString::number(totalTimeLimit) + " 分钟</div>\n";
    
    // 成绩统计
    html += "<h2>🎯 成绩统计</h2>\n";
    html += "<div class=\"stat-box\"><strong>总题数：</strong>" + QString::number(totalQuestions) + " 道</div>\n";
    html += "<div class=\"stat-box\"><strong>正确数：</strong><span class=\"correct\">" + QString::number(correctQuestions) + "</span> 道</div>\n";
    html += "<div class=\"stat-box\"><strong>正确率：</strong>" + QString::number(overallAccuracy, 'f', 1) + "%</div>\n";
    html += "<div class=\"stat-box\"><strong>总得分：</strong>" + QString::number(totalScore) + " 分</div>\n";
    
    // 难度分析表格
    html += "<h2>📈 难度分析</h2>\n";
    html += "<table>\n<tr><th>难度</th><th>题目数</th><th>正确数</th><th>正确率</th><th>平均用时</th></tr>\n";
    
    QList<Difficulty> diffs = {Difficulty::Easy, Difficulty::Medium, Difficulty::Hard};
    for (Difficulty diff : diffs) {
        if (difficultyStats.contains(diff)) {
            const DifficultyStatistics &stats = difficultyStats[diff];
            QString diffName;
            switch (diff) {
                case Difficulty::Easy: diffName = "简单"; break;
                case Difficulty::Medium: diffName = "中等"; break;
                case Difficulty::Hard: diffName = "困难"; break;
            }
            html += QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4%</td><td>%5秒</td></tr>\n")
                .arg(diffName)
                .arg(stats.totalQuestions)
                .arg(stats.correctQuestions)
                .arg(stats.accuracy, 0, 'f', 1)
                .arg(stats.avgTimeSpent);
        }
    }
    html += "</table>\n";
    
    // 知识点分析
    html += "<h2>🎓 知识点分析</h2>\n";
    html += "<table>\n<tr><th>知识点</th><th>题目数</th><th>正确数</th><th>正确率</th></tr>\n";
    
    QList<QString> topics = topicStats.keys();
    for (const QString &topic : topics) {
        const TopicStatistics &stats = topicStats[topic];
        html += QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4%</td></tr>\n")
            .arg(topic)
            .arg(stats.totalQuestions)
            .arg(stats.correctQuestions)
            .arg(stats.accuracy, 0, 'f', 1);
    }
    html += "</table>\n";
    
    // 薄弱知识点
    if (!weakTopics.isEmpty()) {
        html += "<h2>⚠️ 薄弱知识点</h2>\n";
        for (const QString &topic : weakTopics) {
            const TopicStatistics &stats = topicStats[topic];
            html += QString("<div class=\"weak-topic\"><strong>%1</strong> (正确率: %2%)</div>\n")
                .arg(topic)
                .arg(stats.accuracy, 0, 'f', 1);
        }
    }
    
    // 建议
    if (!suggestions.isEmpty()) {
        html += "<h2>💡 建议</h2>\n";
        QString suggestionHtml = suggestions;
        suggestionHtml.replace("\n", "<br>");
        html += "<div class=\"suggestion\">" + suggestionHtml + "</div>\n";
    }
    
    html += "<p style=\"text-align: center; color: #888; margin-top: 40px;\">报告生成时间：" 
        + QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") + "</p>\n";
    
    html += "</div>\n</body>\n</html>";
    
    return html;
}

QJsonObject ExamReport::toJson() const
{
    QJsonObject obj;
    obj["sessionId"] = sessionId;
    obj["examName"] = examName;
    obj["category"] = category;
    obj["startTime"] = startTime.toString(Qt::ISODate);
    obj["endTime"] = endTime.toString(Qt::ISODate);
    obj["totalTimeLimit"] = totalTimeLimit;
    obj["actualTimeSpent"] = actualTimeSpent;
    obj["isTimeout"] = isTimeout;
    obj["totalQuestions"] = totalQuestions;
    obj["attemptedQuestions"] = attemptedQuestions;
    obj["correctQuestions"] = correctQuestions;
    obj["overallAccuracy"] = overallAccuracy;
    obj["totalScore"] = totalScore;
    obj["suggestions"] = suggestions;
    obj["weakTopics"] = QJsonArray::fromStringList(weakTopics);
    
    // 答题详情
    QJsonArray attemptsArray;
    for (const QuestionAttempt &attempt : attempts) {
        attemptsArray.append(attempt.toJson());
    }
    obj["attempts"] = attemptsArray;
    
    return obj;
}
