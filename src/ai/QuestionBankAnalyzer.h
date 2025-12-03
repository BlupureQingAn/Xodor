#ifndef QUESTIONBANKANALYZER_H
#define QUESTIONBANKANALYZER_H

#include <QString>
#include <QMap>
#include <QVector>
#include "../core/Question.h"

// 题库分析结果
struct BankAnalysis {
    QString bankName;
    int totalQuestions = 0;
    
    // 难度分布
    QMap<QString, int> difficultyDistribution;  // "简单": 60, "中等": 70, "困难": 20
    
    // 标签分布
    QMap<QString, int> tagDistribution;         // "数组": 45, "字符串": 30
    
    // 测试数据统计
    double avgTestCases = 0.0;
    int minTestCases = 0;
    int maxTestCases = 0;
    
    // 常见模式
    QStringList commonPatterns;
    
    // 题目组织方式
    QString organizationPattern;  // "按文件夹分类", "单文件多题", "一题一文件"
    
    // 转换为 JSON
    QString toJson() const;
    
    // 从 JSON 加载
    static BankAnalysis fromJson(const QString &json);
    
    // 生成 Markdown 报告
    QString toMarkdown() const;
};

// 题库分析器
class QuestionBankAnalyzer
{
public:
    QuestionBankAnalyzer();
    
    // 分析题库，生成统计报告
    BankAnalysis analyzeBank(const QString &bankPath);
    
    // 分析题目列表
    BankAnalysis analyzeQuestions(const QVector<Question> &questions, const QString &bankName);
    
    // 保存分析结果
    bool saveAnalysis(const QString &bankPath, const BankAnalysis &analysis);
    
    // 加载分析结果
    BankAnalysis loadAnalysis(const QString &bankPath);
    
private:
    // 分析难度分布
    void analyzeDifficultyDistribution(const QVector<Question> &questions, BankAnalysis &analysis);
    
    // 分析标签分布
    void analyzeTagDistribution(const QVector<Question> &questions, BankAnalysis &analysis);
    
    // 分析测试数据
    void analyzeTestCases(const QVector<Question> &questions, BankAnalysis &analysis);
    
    // 检测常见模式
    void detectCommonPatterns(const QVector<Question> &questions, BankAnalysis &analysis);
    
    // 获取分析文件路径
    QString getAnalysisFilePath(const QString &bankPath) const;
};

#endif // QUESTIONBANKANALYZER_H
