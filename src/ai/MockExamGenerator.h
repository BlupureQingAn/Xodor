#ifndef MOCKEXAMGENERATOR_H
#define MOCKEXAMGENERATOR_H

#include <QObject>
#include <QVector>
#include "../core/Question.h"

class OllamaClient;

// 出题规则
struct ExamPattern {
    QString categoryName;           // 分类名称（如 "ccf"）
    int questionsPerExam = 4;       // 每套题目数量
    int timeLimit = 180;            // 时间限制（分钟）
    
    // 难度分布
    QMap<Difficulty, double> difficultyRatio;  // 难度占比
    
    // 知识点分布
    QMap<QString, double> topicRatio;          // 知识点占比
    
    // 代码限制
    int timeLimitPerQuestion = 1000;  // 单题时间限制（ms）
    int memoryLimit = 256;            // 内存限制（MB）
    QStringList supportedLanguages;   // 支持的语言
    
    // 题号规则
    QStringList questionTitlePattern; // 题号命名规则
    
    QString toJson() const;
    static ExamPattern fromJson(const QString &json);
};

// 模拟题生成器
class MockExamGenerator : public QObject
{
    Q_OBJECT
public:
    explicit MockExamGenerator(OllamaClient *aiClient, QObject *parent = nullptr);
    
    // 分析题库，生成出题规则
    ExamPattern analyzeQuestionBank(const QVector<Question> &questions, const QString &categoryName);
    
    // 保存出题规则
    bool savePattern(const QString &bankPath, const ExamPattern &pattern);
    
    // 加载出题规则
    ExamPattern loadPattern(const QString &bankPath);
    
    // 从config目录加载源题库的导入规则
    QJsonObject loadSourceBankRules(const QString &bankName);
    
    // 检查源题库是否有导入规则
    bool hasSourceBankRules(const QString &bankName);
    
    // 生成模拟题（异步）
    void generateMockExam(const ExamPattern &pattern, int examCount = 1);
    
signals:
    void progressUpdated(int percentage, const QString &message);
    void examGenerated(const QVector<Question> &questions, int examIndex);
    void generationComplete(int totalExams);
    void error(const QString &errorMsg);
    
private slots:
    void onAIResponse(const QString &response);
    void onAIError(const QString &error);
    
private:
    QString buildPrompt(const ExamPattern &pattern, int examIndex);
    QVector<Question> parseAIResponse(const QString &response, const ExamPattern &pattern);
    
    OllamaClient *m_aiClient;
    ExamPattern m_currentPattern;
    int m_currentExamIndex;
    int m_totalExams;
};

#endif // MOCKEXAMGENERATOR_H
