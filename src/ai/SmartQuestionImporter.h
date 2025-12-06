#ifndef SMARTQUESTIONIMPORTER_H
#define SMARTQUESTIONIMPORTER_H

#include <QObject>
#include <QVector>
#include <QStringList>
#include "../core/Question.h"

class OllamaClient;
class UniversalQuestionParser;
class QuestionBankAnalyzer;

// 文件分块信息
struct FileChunk {
    QString fileName;
    QString content;
    int chunkIndex;
    int totalChunks;
    int startLine;
    int endLine;
};

// 导入进度信息
struct ImportProgress {
    int totalFiles = 0;
    int processedFiles = 0;
    int totalChunks = 0;
    int processedChunks = 0;
    int totalQuestions = 0;
    QString currentFile;
    QString currentStatus;
};

class SmartQuestionImporter : public QObject
{
    Q_OBJECT
public:
    explicit SmartQuestionImporter(OllamaClient *aiClient, QObject *parent = nullptr);
    ~SmartQuestionImporter();
    
    // 开始导入流程（增强版）
    void startImport(const QString &sourcePath, const QString &targetPath, const QString &bankName);
    
    // 开始导入流程（使用通用解析器）
    void startImportWithUniversalParser(const QString &sourcePath, const QString &targetPath, const QString &bankName);
    
    // 取消导入
    void cancelImport();
    
    // 获取导入的题目
    QVector<Question> getImportedQuestions() const { return m_questions; }
    
signals:
    void progressUpdated(const ImportProgress &progress);
    void fileProcessed(const QString &fileName, int questionCount);
    void chunkProcessed(const QString &fileName, int chunkIndex, int totalChunks);
    void importCompleted(bool success, const QString &message);
    void logMessage(const QString &message);
    
private slots:
    void onAIResponse(const QString &response);
    void onAIError(const QString &error);
    void onStreamProgress(const QString &context, int currentLength, const QString &partialContent);
    
private:
    // 第一步：拷贝文件夹
    bool copyQuestionBank(const QString &sourcePath, const QString &targetPath);
    
    // 第二步：扫描并分析文件
    void scanAndAnalyzeFiles(const QString &path);
    
    // 第三步：智能拆分大文件
    QVector<FileChunk> splitLargeFile(const QString &filePath, const QString &content);
    
    // 第四步：逐个处理文件块
    void processNextChunk();
    
    // 第五步：AI解析单个文件块
    void parseChunkWithAI(const FileChunk &chunk);
    
    // 第六步：解析AI响应并生成测试数据
    void parseAIResponseAndGenerateTests(const QString &response, const FileChunk &chunk);
    
    // 辅助函数
    bool isQuestionBoundary(const QString &line);
    QString buildAIPrompt(const FileChunk &chunk);
    QVector<TestCase> generateTestCases(const Question &question);
    QString fixJsonWithAI(const QString &brokenJson);
    
    OllamaClient *m_aiClient;
    UniversalQuestionParser *m_parser;
    QuestionBankAnalyzer *m_analyzer;
    QString m_targetPath;
    QString m_bankName;
    QVector<FileChunk> m_chunks;
    QVector<Question> m_questions;
    ImportProgress m_progress;
    int m_currentChunkIndex;
    bool m_cancelled;
    bool m_useUniversalParser;
    
    // 新增：保存解析规则和基础题库
    bool saveParseRulesAndQuestionBank();
    
    // 新增：生成出题模式规律
    bool generateExamPattern();
    
    // 新增：保存运行时题库JSON
    bool saveRuntimeQuestionBank();
    
    // MD拆分相关方法
    void generateSplitRules(const QString &content, const QString &fileName);
    QVector<QString> splitByRules(const QString &content, const QJsonObject &rules);
    void extractMetadataForQuestion(const QString &content, int questionIndex);
    bool saveAsMarkdownFile(const QString &content, const QJsonObject &metadata, const QString &filePath);
    QString buildSplitRulesPrompt(const QString &content);
    QString buildMetadataPrompt(const QString &content);
    
    // 拆分规则相关
    QJsonObject m_currentSplitRules;
    QVector<QString> m_splitQuestions;
    int m_currentQuestionIndex;
    
    // 配置参数
    static const int MAX_CHUNK_SIZE = 8000;  // 每个块最大字符数
    static const int MIN_TEST_CASES = 3;     // 最少测试用例数
};

#endif // SMARTQUESTIONIMPORTER_H
