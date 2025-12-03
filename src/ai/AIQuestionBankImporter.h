#ifndef AIQUESTIONBANKIMPORTER_H
#define AIQUESTIONBANKIMPORTER_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QJsonObject>
#include "../core/Question.h"

class OllamaClient;

// 导入阶段
enum class ImportStage {
    Idle,                       // 空闲
    CopyingFiles,              // 复制文件
    AnalyzingFormat,           // AI分析格式
    GeneratingRules,           // 生成解析规则
    ParsingQuestions,          // 解析题目
    GeneratingTestData,        // AI生成测试数据
    OrganizingQuestions,       // 组织题目
    AnalyzingPattern,          // 分析出题规律
    Complete                   // 完成
};

// 解析规则
struct ParseRule {
    QString category;                          // 分类名称
    QStringList titlePatterns;                 // 题目标题模式
    QStringList descriptionPatterns;           // 题目描述模式
    QStringList inputPatterns;                 // 输入格式模式
    QStringList outputPatterns;                // 输出格式模式
    QStringList testCasePatterns;              // 测试用例模式
    QStringList constraintPatterns;            // 约束条件模式
    QStringList splitPatterns;                 // 题目分割模式
    
    QJsonObject toJson() const;
    static ParseRule fromJson(const QJsonObject &json);
};

// AI驱动的题库导入器
class AIQuestionBankImporter : public QObject
{
    Q_OBJECT
public:
    explicit AIQuestionBankImporter(OllamaClient *aiClient, QObject *parent = nullptr);
    
    // 开始导入
    void startImport(const QString &sourcePath, const QString &categoryName);
    
    // 取消导入
    void cancelImport();
    
    // 获取当前阶段
    ImportStage currentStage() const { return m_currentStage; }
    
signals:
    // 进度更新
    void stageChanged(ImportStage stage, const QString &message);
    void progressUpdated(int percentage, const QString &message);
    
    // 完成信号
    void importComplete(const QString &categoryName, int questionCount);
    void importFailed(const QString &error);
    
private slots:
    void onAIResponse(const QString &response);
    void onAIError(const QString &error);
    
private:
    // 阶段处理
    void processStage1_CopyFiles();
    void processStage2_AnalyzeFormat();
    void processStage3_GenerateRules();
    void processStage4_ParseQuestions();
    void processStage5_GenerateTestData();
    void processStage6_OrganizeQuestions();
    void processStage7_AnalyzePattern();
    void processStage8_Complete();
    
    // AI辅助方法
    QString buildFormatAnalysisPrompt(const QStringList &sampleFiles);
    QString buildParsePrompt(const QString &fileContent, const ParseRule &rule);
    QString buildTestDataPrompt(const Question &question, int additionalCount);
    QString buildPatternAnalysisPrompt(const QVector<Question> &questions);
    
    // 解析方法
    ParseRule parseFormatAnalysisResponse(const QString &response);
    QVector<Question> parseQuestionsResponse(const QString &response);
    QVector<TestCase> parseTestDataResponse(const QString &response);
    
    // 文件操作
    bool copyToOriginalBank(const QString &sourcePath, const QString &categoryName);
    QStringList getSampleFiles(const QString &path, int count = 3);
    bool saveParseRule(const ParseRule &rule);
    bool saveQuestions(const QVector<Question> &questions, const QString &categoryName);
    bool savePattern(const QString &categoryName, const QJsonObject &pattern);
    
    // 路径管理
    QString getOriginalBankPath(const QString &category) const;
    QString getBaseBankPath(const QString &category) const;
    QString getConfigPath(const QString &category) const;
    
    OllamaClient *m_aiClient;
    ImportStage m_currentStage;
    bool m_cancelled;
    
    // 导入数据
    QString m_sourcePath;
    QString m_categoryName;
    QStringList m_mdFiles;
    ParseRule m_parseRule;
    QVector<Question> m_questions;
    int m_currentFileIndex;
    int m_currentQuestionIndex;
};

#endif // AIQUESTIONBANKIMPORTER_H
