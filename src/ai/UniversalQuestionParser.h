#ifndef UNIVERSALQUESTIONPARSER_H
#define UNIVERSALQUESTIONPARSER_H

#include <QString>
#include <QStringList>
#include <QVector>
#include <QRegularExpression>
#include "../core/Question.h"

// 解析模式
struct ParsePattern {
    // 题目分隔符模式
    QStringList questionSeparators;  // 如: "# ", "## 题目", "第X题", "Problem"
    
    // 测试数据模式
    QStringList inputKeywords;       // 如: "输入:", "Input:", "输入："
    QStringList outputKeywords;      // 如: "输出:", "Output:", "输出："
    QStringList exampleKeywords;     // 如: "示例", "Example", "测试用例"
    
    // 元信息模式
    QStringList difficultyKeywords;  // 如: "难度:", "Difficulty:"
    QStringList tagKeywords;         // 如: "标签:", "Tags:"
    QStringList limitKeywords;       // 如: "时间限制:", "Time Limit:"
    
    // 格式特征
    bool hasMultipleQuestions;       // 单文件是否包含多题
    bool hasStructuredExamples;      // 是否有结构化示例
    QString encoding;                // 文件编码
};

// 题目元信息
struct QuestionMetadata {
    QString title;
    Difficulty difficulty;
    QStringList tags;
    QString timeLimit;
    QString memoryLimit;
    QString description;
};

// 通用题目解析器
class UniversalQuestionParser
{
public:
    UniversalQuestionParser();
    
    // 分析文件格式，自动识别题目结构
    ParsePattern analyzeFormat(const QString &content);
    
    // 按识别的模式解析题目
    QVector<Question> parseContent(const QString &content, const ParsePattern &pattern);
    
    // 智能拆分单文件内的多道题目
    QStringList splitMultipleQuestions(const QString &content);
    
    // 提取测试数据（智能配对输入输出）
    QVector<TestCase> extractTestCases(const QString &content);
    
    // 提取题目元信息
    QuestionMetadata extractMetadata(const QString &content);
    
    // 解析单道题目
    Question parseSingleQuestion(const QString &content);
    
private:
    // 检测题目分隔符
    QStringList detectQuestionSeparators(const QString &content);
    
    // 检测测试数据格式
    void detectTestCaseFormat(const QString &content, ParsePattern &pattern);
    
    // 检测元信息格式
    void detectMetadataFormat(const QString &content, ParsePattern &pattern);
    
    // 判断是否为题目边界
    bool isQuestionBoundary(const QString &line);
    
    // 提取难度
    Difficulty parseDifficulty(const QString &text);
    
    // 提取标签
    QStringList parseTags(const QString &text);
    
    // 清理文本
    QString cleanText(const QString &text);
    
    // 智能配对输入输出
    QVector<TestCase> pairInputOutput(const QStringList &inputs, const QStringList &outputs);
    
    // 生成测试用例描述
    QString generateTestCaseDescription(int index, int total);
    
    // 常用的题目分隔符模式
    QVector<QRegularExpression> m_questionPatterns;
    
    // 常用的测试数据模式
    QVector<QRegularExpression> m_inputPatterns;
    QVector<QRegularExpression> m_outputPatterns;
    QVector<QRegularExpression> m_examplePatterns;
    
    // 常用的元信息模式
    QVector<QRegularExpression> m_difficultyPatterns;
    QVector<QRegularExpression> m_tagPatterns;
    QVector<QRegularExpression> m_limitPatterns;
};

#endif // UNIVERSALQUESTIONPARSER_H
