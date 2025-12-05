#ifndef COMPILERRUNNER_H
#define COMPILERRUNNER_H

#include <QObject>
#include <QProcess>
#include "Question.h"

struct CompileResult {
    bool success;
    QString output;
    QString error;
    QString executablePath;  // 生成的可执行文件路径
};

// 测试失败原因
enum class TestFailureReason {
    None,                  // 通过
    WrongAnswer,           // 答案错误
    RuntimeError,          // 运行时错误
    TimeLimitExceeded,     // 超时
    MemoryLimitExceeded,   // 内存超限
    CompileError           // 编译错误
};

struct TestResult {
    bool passed;
    QString input;
    QString expectedOutput;
    QString actualOutput;
    QString error;
    QString description;           // 测试用例描述
    int caseIndex;                 // 测试用例编号（从1开始）
    TestFailureReason failureReason; // 失败原因
    int executionTime;             // 执行时间（毫秒）
    bool isAIGenerated;            // 是否 AI 生成的测试数据
};

class CompilerRunner : public QObject
{
    Q_OBJECT
public:
    explicit CompilerRunner(QObject *parent = nullptr);
    
    void setCompilerPath(const QString &path);
    CompileResult compile(const QString &code);
    QVector<TestResult> runTests(const QString &executablePath, const QVector<TestCase> &testCases);
    
signals:
    void compileFinished(const CompileResult &result);
    void testFinished(const QVector<TestResult> &results);
    
private:
    QString m_compilerPath;
    QString createTempFile(const QString &code);
};

#endif // COMPILERRUNNER_H
