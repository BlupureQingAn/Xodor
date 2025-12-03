#include "CompilerRunner.h"
#include <QFile>
#include <QDir>
#include <QTemporaryFile>
#include <QProcess>
#include <QElapsedTimer>

CompilerRunner::CompilerRunner(QObject *parent)
    : QObject(parent)
    , m_compilerPath("g++")
{
}

void CompilerRunner::setCompilerPath(const QString &path)
{
    m_compilerPath = path;
}

QString CompilerRunner::createTempFile(const QString &code)
{
    QTemporaryFile tempFile(QDir::tempPath() + "/code_XXXXXX.cpp");
    tempFile.setAutoRemove(false);
    if (tempFile.open()) {
        tempFile.write(code.toUtf8());
        tempFile.close();
        return tempFile.fileName();
    }
    return QString();
}

CompileResult CompilerRunner::compile(const QString &code)
{
    CompileResult result;
    result.success = false;
    
    QString sourceFile = createTempFile(code);
    if (sourceFile.isEmpty()) {
        result.error = "无法创建临时文件";
        return result;
    }
    
    QString exeFile = sourceFile;
    exeFile.replace(".cpp", ".exe");
    
    QProcess process;
    QStringList args;
    args << sourceFile << "-o" << exeFile << "-std=c++17";
    
    process.start(m_compilerPath, args);
    process.waitForFinished(10000);
    
    result.output = process.readAllStandardOutput();
    result.error = process.readAllStandardError();
    result.success = (process.exitCode() == 0);
    
    return result;
}

QVector<TestResult> CompilerRunner::runTests(const QString &executablePath, const QVector<TestCase> &testCases)
{
    QVector<TestResult> results;
    
    for (int i = 0; i < testCases.size(); ++i) {
        const auto &testCase = testCases[i];
        TestResult result;
        result.input = testCase.input;
        result.expectedOutput = testCase.expectedOutput;
        result.description = testCase.description;
        result.caseIndex = i + 1;  // 从1开始编号
        result.isAIGenerated = testCase.isAIGenerated;  // 标记是否AI生成
        result.failureReason = TestFailureReason::None;
        result.executionTime = 0;
        
        QProcess process;
        
        // 记录开始时间
        QElapsedTimer timer;
        timer.start();
        
        process.start(executablePath);
        if (!process.waitForStarted(1000)) {
            result.passed = false;
            result.error = "程序启动失败";
            result.failureReason = TestFailureReason::RuntimeError;
            result.executionTime = timer.elapsed();
            results.append(result);
            continue;
        }
        
        process.write(testCase.input.toUtf8());
        process.closeWriteChannel();
        
        // 等待程序完成，超时时间5秒
        bool finished = process.waitForFinished(5000);
        result.executionTime = timer.elapsed();
        
        if (!finished) {
            // 超时
            process.kill();
            result.passed = false;
            result.error = "程序执行超时（超过5秒）";
            result.failureReason = TestFailureReason::TimeLimitExceeded;
            result.actualOutput = process.readAllStandardOutput().trimmed();
        } else if (process.exitCode() != 0) {
            // 运行时错误
            result.passed = false;
            result.actualOutput = process.readAllStandardOutput().trimmed();
            result.error = process.readAllStandardError();
            result.failureReason = TestFailureReason::RuntimeError;
        } else {
            // 正常完成
            result.actualOutput = process.readAllStandardOutput().trimmed();
            result.error = process.readAllStandardError();
            result.passed = (result.actualOutput == result.expectedOutput.trimmed());
            
            if (!result.passed) {
                result.failureReason = TestFailureReason::WrongAnswer;
            }
        }
        
        results.append(result);
    }
    
    return results;
}
