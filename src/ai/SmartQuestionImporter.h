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
    // 阶段标识
    enum Stage {
        Scanning,      // 扫描文件阶段 (0-10%)
        Parsing,       // AI解析阶段 (10-95%)
        Saving,        // 保存完成阶段 (95-100%)
        Complete       // 全部完成 (100%)
    };
    
    Stage currentStage = Scanning;
    
    // 文件统计
    int totalFiles = 0;
    int processedFiles = 0;      // 已扫描的文件数（扫描阶段使用）
    int currentFileIndex = 0;     // 当前正在处理的文件索引（AI解析阶段使用，从0开始）
    
    // 题目统计
    int totalQuestions = 0;       // 已识别的题目总数
    
    // 当前状态
    QString currentFile;
    QString currentStatus;
    
    // 保存进度（用于Saving阶段）
    int saveProgress = 0;  // 0-100
    
    // 计算进度百分比
    int calculatePercentage() const;
};

// 导入结果信息
struct ImportResult {
    bool success = false;                           // 是否成功
    int totalQuestions = 0;                         // 总题目数
    QMap<QString, int> questionsByFile;             // 按源文件分类统计
    QMap<QString, int> questionsByDifficulty;       // 按难度分类统计
    QString basePath;                               // 保存路径
    QString errorMessage;                           // 错误信息
    QStringList warnings;                           // 警告信息列表
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
    void importCompleted(const ImportResult &result);
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
    
    // 新增：递归拆分处理
    void parseAIResponseRecursive(const QString &response, const FileChunk &chunk);
    
    // 辅助函数
    bool isQuestionBoundary(const QString &line);
    QString buildAIPrompt(const FileChunk &chunk);
    QVector<TestCase> generateTestCases(const Question &question);
    QVector<TestCase> extractTestCasesFromMarkdown(const QString &markdown);
    QString fixJsonWithAI(const QString &brokenJson);
    ImportResult buildImportResult(bool success, const QString &errorMessage = QString());
    
    // 进度管理方法
    void updateProgress();
    void enterScanningStage();
    void enterParsingStage();
    void enterSavingStage();
    void enterCompleteStage();
    
    // 新增：行号提取辅助函数
    QString extractLines(const QString &content, int startLine, int endLine);
    QString extractLinesFrom(const QString &content, int startLine);
    QVector<TestCase> generateTestCasesFromHints(const QJsonArray &hints, const QString &questionContent);
    
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
    bool m_isRecursiveProcessing;  // 标志是否在递归处理中
    QSet<QString> m_processedTitles;  // 记录已处理的题目标题（当前文件）
    int m_recursiveDepth;  // 当前递归深度
    int m_lastContentLength;  // 上次处理的内容长度
    FileChunk m_currentProcessingChunk;  // 当前正在处理的chunk（用于递归）
    
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
