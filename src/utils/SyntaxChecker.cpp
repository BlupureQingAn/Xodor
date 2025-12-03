#include "SyntaxChecker.h"
#include <QFile>
#include <QDir>
#include <QRegularExpression>
#include <QDebug>

SyntaxChecker::SyntaxChecker(QObject *parent)
    : QObject(parent)
    , m_process(nullptr)
{
    m_checkTimer = new QTimer(this);
    m_checkTimer->setSingleShot(true);
    m_checkTimer->setInterval(500);  // 500毫秒延迟，更快响应
    
    connect(m_checkTimer, &QTimer::timeout, this, &SyntaxChecker::performCheck);
    
    m_tempFilePath = QDir::temp().filePath("syntax_check_temp.cpp");
}

void SyntaxChecker::checkCode(const QString &code, const QString &compiler)
{
    // 如果代码为空，清空错误列表
    if (code.trimmed().isEmpty()) {
        emit errorsFound(QVector<SyntaxError>());
        return;
    }
    
    m_pendingCode = code;
    m_compiler = compiler;
    
    // 重启定时器（延迟检查，避免频繁触发）
    m_checkTimer->start();
}

void SyntaxChecker::performCheck()
{
    if (m_pendingCode.isEmpty() || m_compiler.isEmpty()) {
        qDebug() << "[SyntaxChecker] Skipping check: code or compiler empty";
        return;
    }
    
    // 如果代码没有变化，跳过检查
    if (m_pendingCode == m_lastCheckedCode) {
        qDebug() << "[SyntaxChecker] Skipping check: code unchanged";
        return;
    }
    
    qDebug() << "[SyntaxChecker] Performing syntax check with compiler:" << m_compiler;
    
    // 更新缓存
    m_lastCheckedCode = m_pendingCode;
    
    // 写入临时文件
    QFile file(m_tempFilePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "[SyntaxChecker] ERROR: Cannot create temp file:" << m_tempFilePath;
        return;
    }
    file.write(m_pendingCode.toUtf8());
    file.close();
    
    qDebug() << "[SyntaxChecker] Temp file created:" << m_tempFilePath;
    
    // 停止之前的进程
    if (m_process && m_process->state() != QProcess::NotRunning) {
        m_process->kill();
        m_process->waitForFinished(100);
    }
    
    // 创建新进程
    if (m_process) {
        m_process->deleteLater();
    }
    m_process = new QProcess(this);
    
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &SyntaxChecker::onProcessFinished);
    
    // 运行编译器语法检查
    QStringList args;
    args << "-fsyntax-only"  // 只检查语法
         << "-fdiagnostics-color=never"  // 不使用颜色
         << "-std=c++17"
         << m_tempFilePath;
    
    m_process->start(m_compiler, args);
}

void SyntaxChecker::onProcessFinished(int exitCode, QProcess::ExitStatus status)
{
    qDebug() << "[SyntaxChecker] Process finished, exit code:" << exitCode << "status:" << status;
    
    if (status != QProcess::NormalExit) {
        qDebug() << "[SyntaxChecker] Process did not exit normally";
        emit checkFinished();
        return;
    }
    
    QString output = m_process->readAllStandardError();
    qDebug() << "[SyntaxChecker] Compiler output:" << output;
    
    QVector<SyntaxError> errors = parseCompilerOutput(output);
    qDebug() << "[SyntaxChecker] Found" << errors.size() << "errors/warnings";
    
    emit errorsFound(errors);
    emit checkFinished();
}

QVector<SyntaxError> SyntaxChecker::parseCompilerOutput(const QString &output)
{
    QVector<SyntaxError> errors;
    
    // 解析GCC/MinGW错误格式：file:line:column: error/warning: message
    QRegularExpression regex(R"(.*:(\d+):(\d+):\s+(error|warning):\s+(.+))");
    
    QStringList lines = output.split('\n');
    qDebug() << "[SyntaxChecker] Parsing" << lines.size() << "lines of output";
    
    for (const QString &line : lines) {
        if (line.trimmed().isEmpty()) continue;
        
        QRegularExpressionMatch match = regex.match(line);
        if (match.hasMatch()) {
            SyntaxError error;
            error.line = match.captured(1).toInt();
            error.column = match.captured(2).toInt();
            error.type = match.captured(3);
            error.message = match.captured(4).trimmed();
            
            qDebug() << "[SyntaxChecker] Parsed error:" << error.type << "at line" << error.line << ":" << error.message;
            errors.append(error);
        }
    }
    
    qDebug() << "[SyntaxChecker] Total errors parsed:" << errors.size();
    return errors;
}
