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
    
    // 添加UTF-8编码支持，防止中文乱码
    args << "-finput-charset=UTF-8" << "-fexec-charset=UTF-8";
    
    process.start(m_compilerPath, args);
    process.waitForFinished(10000);
    
    result.output = process.readAllStandardOutput();
    result.error = process.readAllStandardError();
    result.success = (process.exitCode() == 0);
    
    // 保存可执行文件路径
    if (result.success) {
        result.executablePath = exeFile;
    }
    
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
            result.error = QString("程序启动失败：%1").arg(process.errorString());
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
            // 运行时错误（非零退出码）
            result.passed = false;
            result.actualOutput = process.readAllStandardOutput().trimmed();
            QString stderrOutput = process.readAllStandardError().trimmed();
            
            // 提供更详细的错误信息
            if (!stderrOutput.isEmpty()) {
                result.error = QString("运行时错误（退出码 %1）：%2")
                    .arg(process.exitCode())
                    .arg(stderrOutput);
            } else {
                result.error = QString("运行时错误（退出码 %1）：程序异常退出")
                    .arg(process.exitCode());
            }
            result.failureReason = TestFailureReason::RuntimeError;
        } else {
            // 正常完成（退出码为0）
            QString rawOutput = process.readAllStandardOutput();
            result.error = process.readAllStandardError().trimmed();
            
            // 标准化输出：去除首尾空白，统一行尾
            auto normalizeOutput = [](const QString &output) -> QString {
                QString normalized = output.trimmed();
                // 统一换行符（Windows的\r\n转为\n）
                normalized.replace("\r\n", "\n");
                // 去除每行末尾的空白
                QStringList lines = normalized.split('\n');
                for (QString &line : lines) {
                    line = line.trimmed();
                }
                return lines.join('\n');
            };
            
            result.actualOutput = rawOutput.trimmed();
            QString normalizedActual = normalizeOutput(rawOutput);
            QString normalizedExpected = normalizeOutput(result.expectedOutput);
            
            // 检查输出是否匹配（使用标准化后的字符串）
            if (normalizedActual == normalizedExpected) {
                result.passed = true;
            } else {
                result.passed = false;
                result.failureReason = TestFailureReason::WrongAnswer;
                
                // 提供更详细的错误提示
                if (normalizedActual.isEmpty()) {
                    result.error = "程序没有产生任何输出。请检查：\n"
                                   "1. 是否读取了输入数据？\n"
                                   "2. 是否输出了结果？\n"
                                   "3. 输出格式是否正确？";
                } else {
                    // 按行比较，找出差异
                    QStringList actualLines = normalizedActual.split('\n');
                    QStringList expectedLines = normalizedExpected.split('\n');
                    
                    if (actualLines.size() < expectedLines.size()) {
                        result.error = QString("输出不完整（期望%1行，实际%2行）。请检查：\n"
                                             "1. 是否处理了所有输入数据？\n"
                                             "2. 是否输出了所有结果？")
                                       .arg(expectedLines.size()).arg(actualLines.size());
                    } else if (actualLines.size() > expectedLines.size()) {
                        result.error = QString("输出过多（期望%1行，实际%2行）。请检查：\n"
                                             "1. 是否有多余的调试输出？\n"
                                             "2. 输出格式是否正确？")
                                       .arg(expectedLines.size()).arg(actualLines.size());
                    } else {
                        // 行数相同但内容不同，找出第一个不同的行
                        int diffLine = -1;
                        for (int i = 0; i < actualLines.size(); ++i) {
                            if (actualLines[i] != expectedLines[i]) {
                                diffLine = i + 1;
                                break;
                            }
                        }
                        result.error = QString("输出不匹配（第%1行不同）。请检查：\n"
                                             "1. 输出格式是否正确？\n"
                                             "2. 计算逻辑是否正确？\n"
                                             "期望：%3\n"
                                             "实际：%4")
                                       .arg(diffLine)
                                       .arg(expectedLines[diffLine - 1])
                                       .arg(actualLines[diffLine - 1]);
                    }
                }
            }
        }
        
        results.append(result);
    }
    
    return results;
}
