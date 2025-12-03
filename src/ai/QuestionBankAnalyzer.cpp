#include "QuestionBankAnalyzer.h"
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

QuestionBankAnalyzer::QuestionBankAnalyzer()
{
}

BankAnalysis QuestionBankAnalyzer::analyzeQuestions(const QVector<Question> &questions, const QString &bankName)
{
    BankAnalysis analysis;
    analysis.bankName = bankName;
    analysis.totalQuestions = questions.size();
    
    if (questions.isEmpty()) {
        return analysis;
    }
    
    // 分析难度分布
    analyzeDifficultyDistribution(questions, analysis);
    
    // 分析标签分布
    analyzeTagDistribution(questions, analysis);
    
    // 分析测试数据
    analyzeTestCases(questions, analysis);
    
    // 检测常见模式
    detectCommonPatterns(questions, analysis);
    
    qDebug() << "Bank analysis completed:" << analysis.totalQuestions << "questions";
    return analysis;
}

void QuestionBankAnalyzer::analyzeDifficultyDistribution(const QVector<Question> &questions, BankAnalysis &analysis)
{
    analysis.difficultyDistribution.clear();
    analysis.difficultyDistribution["简单"] = 0;
    analysis.difficultyDistribution["中等"] = 0;
    analysis.difficultyDistribution["困难"] = 0;
    
    for (const Question &q : questions) {
        QString diffStr;
        switch (q.difficulty()) {
            case Difficulty::Easy:
                diffStr = "简单";
                break;
            case Difficulty::Medium:
                diffStr = "中等";
                break;
            case Difficulty::Hard:
                diffStr = "困难";
                break;
        }
        analysis.difficultyDistribution[diffStr]++;
    }
}

void QuestionBankAnalyzer::analyzeTagDistribution(const QVector<Question> &questions, BankAnalysis &analysis)
{
    analysis.tagDistribution.clear();
    
    for (const Question &q : questions) {
        QStringList tags = q.tags();
        for (const QString &tag : tags) {
            if (!tag.isEmpty()) {
                analysis.tagDistribution[tag]++;
            }
        }
    }
}

void QuestionBankAnalyzer::analyzeTestCases(const QVector<Question> &questions, BankAnalysis &analysis)
{
    if (questions.isEmpty()) {
        return;
    }
    
    int totalTestCases = 0;
    analysis.minTestCases = INT_MAX;
    analysis.maxTestCases = 0;
    
    for (const Question &q : questions) {
        int count = q.testCases().size();
        totalTestCases += count;
        
        if (count < analysis.minTestCases) {
            analysis.minTestCases = count;
        }
        if (count > analysis.maxTestCases) {
            analysis.maxTestCases = count;
        }
    }
    
    analysis.avgTestCases = static_cast<double>(totalTestCases) / questions.size();
    
    if (analysis.minTestCases == INT_MAX) {
        analysis.minTestCases = 0;
    }
}

void QuestionBankAnalyzer::detectCommonPatterns(const QVector<Question> &questions, BankAnalysis &analysis)
{
    analysis.commonPatterns.clear();
    
    // 检测是否所有题目都有测试数据
    bool allHaveTests = true;
    for (const Question &q : questions) {
        if (q.testCases().isEmpty()) {
            allHaveTests = false;
            break;
        }
    }
    if (allHaveTests) {
        analysis.commonPatterns.append("所有题目包含测试数据");
    }
    
    // 检测是否有统一的难度分布
    int easyCount = analysis.difficultyDistribution["简单"];
    int mediumCount = analysis.difficultyDistribution["中等"];
    int hardCount = analysis.difficultyDistribution["困难"];
    int total = questions.size();
    
    if (total > 0) {
        double easyPercent = (double)easyCount / total * 100;
        double mediumPercent = (double)mediumCount / total * 100;
        double hardPercent = (double)hardCount / total * 100;
        
        if (easyPercent > 50) {
            analysis.commonPatterns.append("以简单题为主");
        } else if (mediumPercent > 50) {
            analysis.commonPatterns.append("以中等题为主");
        } else if (hardPercent > 50) {
            analysis.commonPatterns.append("以困难题为主");
        } else {
            analysis.commonPatterns.append("难度分布均衡");
        }
    }
    
    // 检测测试数据规模
    if (analysis.avgTestCases >= 5) {
        analysis.commonPatterns.append("测试数据充足");
    } else if (analysis.avgTestCases >= 3) {
        analysis.commonPatterns.append("测试数据适中");
    } else {
        analysis.commonPatterns.append("测试数据较少");
    }
    
    // 检测标签使用情况
    if (analysis.tagDistribution.size() > 10) {
        analysis.commonPatterns.append("知识点覆盖广泛");
    } else if (analysis.tagDistribution.size() > 5) {
        analysis.commonPatterns.append("知识点覆盖适中");
    }
}

BankAnalysis QuestionBankAnalyzer::analyzeBank(const QString &bankPath)
{
    BankAnalysis analysis;
    
    // 尝试加载已有的分析结果
    QString analysisPath = getAnalysisFilePath(bankPath);
    if (QFile::exists(analysisPath)) {
        analysis = loadAnalysis(bankPath);
        if (analysis.totalQuestions > 0) {
            qDebug() << "Loaded existing analysis for" << bankPath;
            return analysis;
        }
    }
    
    // 如果没有分析结果，返回空分析
    QDir dir(bankPath);
    analysis.bankName = dir.dirName();
    
    return analysis;
}

bool QuestionBankAnalyzer::saveAnalysis(const QString &bankPath, const BankAnalysis &analysis)
{
    QString filePath = getAnalysisFilePath(bankPath);
    
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Failed to save analysis:" << filePath;
        return false;
    }
    
    file.write(analysis.toJson().toUtf8());
    file.close();
    
    // 同时保存 Markdown 报告
    QString mdPath = bankPath + "/bank_analysis.md";
    QFile mdFile(mdPath);
    if (mdFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        mdFile.write(analysis.toMarkdown().toUtf8());
        mdFile.close();
    }
    
    qDebug() << "Analysis saved to" << filePath;
    return true;
}

BankAnalysis QuestionBankAnalyzer::loadAnalysis(const QString &bankPath)
{
    QString filePath = getAnalysisFilePath(bankPath);
    
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to load analysis:" << filePath;
        return BankAnalysis();
    }
    
    QString json = file.readAll();
    file.close();
    
    return BankAnalysis::fromJson(json);
}

QString QuestionBankAnalyzer::getAnalysisFilePath(const QString &bankPath) const
{
    return bankPath + "/bank_analysis.json";
}

QString BankAnalysis::toJson() const
{
    QJsonObject obj;
    obj["bankName"] = bankName;
    obj["totalQuestions"] = totalQuestions;
    obj["avgTestCases"] = avgTestCases;
    obj["minTestCases"] = minTestCases;
    obj["maxTestCases"] = maxTestCases;
    obj["organizationPattern"] = organizationPattern;
    
    // 难度分布
    QJsonObject diffObj;
    for (auto it = difficultyDistribution.begin(); it != difficultyDistribution.end(); ++it) {
        diffObj[it.key()] = it.value();
    }
    obj["difficultyDistribution"] = diffObj;
    
    // 标签分布
    QJsonObject tagObj;
    for (auto it = tagDistribution.begin(); it != tagDistribution.end(); ++it) {
        tagObj[it.key()] = it.value();
    }
    obj["tagDistribution"] = tagObj;
    
    // 常见模式
    QJsonArray patternArray;
    for (const QString &pattern : commonPatterns) {
        patternArray.append(pattern);
    }
    obj["commonPatterns"] = patternArray;
    
    QJsonDocument doc(obj);
    return doc.toJson(QJsonDocument::Indented);
}

BankAnalysis BankAnalysis::fromJson(const QString &json)
{
    BankAnalysis analysis;
    
    QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
    if (!doc.isObject()) {
        return analysis;
    }
    
    QJsonObject obj = doc.object();
    analysis.bankName = obj["bankName"].toString();
    analysis.totalQuestions = obj["totalQuestions"].toInt();
    analysis.avgTestCases = obj["avgTestCases"].toDouble();
    analysis.minTestCases = obj["minTestCases"].toInt();
    analysis.maxTestCases = obj["maxTestCases"].toInt();
    analysis.organizationPattern = obj["organizationPattern"].toString();
    
    // 难度分布
    QJsonObject diffObj = obj["difficultyDistribution"].toObject();
    for (auto it = diffObj.begin(); it != diffObj.end(); ++it) {
        analysis.difficultyDistribution[it.key()] = it.value().toInt();
    }
    
    // 标签分布
    QJsonObject tagObj = obj["tagDistribution"].toObject();
    for (auto it = tagObj.begin(); it != tagObj.end(); ++it) {
        analysis.tagDistribution[it.key()] = it.value().toInt();
    }
    
    // 常见模式
    QJsonArray patternArray = obj["commonPatterns"].toArray();
    for (const QJsonValue &val : patternArray) {
        analysis.commonPatterns.append(val.toString());
    }
    
    return analysis;
}

QString BankAnalysis::toMarkdown() const
{
    QString md;
    md += "# " + bankName + " 题库分析报告\n\n";
    
    md += "## 📊 总体统计\n\n";
    md += QString("- 总题数：%1 题\n").arg(totalQuestions);
    md += QString("- 平均测试用例数：%.1f 组\n").arg(avgTestCases);
    md += QString("- 测试用例范围：%1 - %2 组\n\n").arg(minTestCases).arg(maxTestCases);
    
    md += "## 📈 难度分布\n\n";
    md += "| 难度 | 数量 | 占比 |\n";
    md += "|------|------|------|\n";
    for (auto it = difficultyDistribution.begin(); it != difficultyDistribution.end(); ++it) {
        double percent = totalQuestions > 0 ? (double)it.value() / totalQuestions * 100 : 0;
        md += QString("| %1 | %2 | %.1f%% |\n").arg(it.key()).arg(it.value()).arg(percent);
    }
    md += "\n";
    
    md += "## 🏷️ 知识点分布\n\n";
    if (!tagDistribution.isEmpty()) {
        md += "| 知识点 | 题目数 |\n";
        md += "|--------|--------|\n";
        
        // 按题目数量排序
        QList<QPair<QString, int>> sortedTags;
        for (auto it = tagDistribution.begin(); it != tagDistribution.end(); ++it) {
            sortedTags.append(qMakePair(it.key(), it.value()));
        }
        std::sort(sortedTags.begin(), sortedTags.end(), 
                  [](const QPair<QString, int> &a, const QPair<QString, int> &b) {
                      return a.second > b.second;
                  });
        
        // 只显示前 10 个
        int count = qMin(10, sortedTags.size());
        for (int i = 0; i < count; ++i) {
            md += QString("| %1 | %2 |\n").arg(sortedTags[i].first).arg(sortedTags[i].second);
        }
        md += "\n";
    } else {
        md += "暂无标签信息\n\n";
    }
    
    md += "## 🎯 题库特征\n\n";
    if (!commonPatterns.isEmpty()) {
        for (const QString &pattern : commonPatterns) {
            md += "- " + pattern + "\n";
        }
    } else {
        md += "暂无特征分析\n";
    }
    
    md += "\n---\n\n";
    md += "*此报告由系统自动生成*\n";
    
    return md;
}
