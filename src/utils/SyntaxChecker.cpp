#include "SyntaxChecker.h"
#include <QFile>
#include <QDir>
#include <QRegularExpression>
#include <QDebug>
#include <algorithm>

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
    
    // 清理代码：统一换行符为 \n，避免 \r 导致的行号问题
    m_pendingCode = code;
    m_pendingCode.replace("\r\n", "\n");  // Windows 换行符
    m_pendingCode.replace("\r", "\n");    // Mac 旧式换行符
    
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
    qDebug() << "[SyntaxChecker] Code line count:" << m_pendingCode.count('\n') + 1;
    
    // 调试：输出前10行代码
    QStringList codeLines = m_pendingCode.split('\n');
    qDebug() << "[SyntaxChecker] First 10 lines of code:";
    for (int i = 0; i < qMin(10, codeLines.size()); ++i) {
        qDebug() << QString("  Line %1: %2").arg(i + 1).arg(codeLines[i]);
    }
    
    // 更新缓存
    m_lastCheckedCode = m_pendingCode;
    
    // 写入临时文件（使用二进制模式，避免换行符转换）
    QFile file(m_tempFilePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "[SyntaxChecker] ERROR: Cannot create temp file:" << m_tempFilePath;
        return;
    }
    file.write(m_pendingCode.toUtf8());
    file.close();
    
    qDebug() << "[SyntaxChecker] Temp file created:" << m_tempFilePath;
    
    // 验证临时文件内容（用二进制模式读取，查看实际内容）
    QFile verifyFile(m_tempFilePath);
    if (verifyFile.open(QIODevice::ReadOnly)) {
        QByteArray fileBytes = verifyFile.readAll();
        QString fileContent = QString::fromUtf8(fileBytes);
        int fileLineCount = fileContent.count('\n') + 1;
        qDebug() << "[SyntaxChecker] Temp file actual line count:" << fileLineCount;
        qDebug() << "[SyntaxChecker] Temp file size:" << fileBytes.size() << "bytes";
        qDebug() << "[SyntaxChecker] Contains \\r:" << fileContent.contains('\r');
        
        // 输出临时文件的前10行
        QStringList fileLines = fileContent.split('\n');
        qDebug() << "[SyntaxChecker] Temp file first 10 lines:";
        for (int i = 0; i < qMin(10, fileLines.size()); ++i) {
            qDebug() << QString("  TempLine %1: %2").arg(i + 1).arg(fileLines[i]);
        }
        verifyFile.close();
    }
    
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

QString SyntaxChecker::improveErrorMessage(const QString &originalMessage, int line)
{
    // 智能改进错误信息，提供更准确的提示
    QString improved = originalMessage;
    
    // 规则1: "expected initializer before" 通常是上一行缺少分号
    if (originalMessage.contains("expected initializer before", Qt::CaseInsensitive)) {
        if (originalMessage.contains("'return'", Qt::CaseInsensitive) ||
            originalMessage.contains("'if'", Qt::CaseInsensitive) ||
            originalMessage.contains("'for'", Qt::CaseInsensitive) ||
            originalMessage.contains("'while'", Qt::CaseInsensitive) ||
            originalMessage.contains("'int'", Qt::CaseInsensitive) ||
            originalMessage.contains("'cout'", Qt::CaseInsensitive) ||
            originalMessage.contains("'cin'", Qt::CaseInsensitive)) {
            improved = QString("上一行可能缺少分号 ';'");
        }
    }
    
    // 规则2: "expected ';' before" 直接指出缺少分号
    else if (originalMessage.contains("expected ';' before", Qt::CaseInsensitive)) {
        QRegularExpression beforeRegex("before\\s+(.+)");
        QRegularExpressionMatch match = beforeRegex.match(originalMessage);
        if (match.hasMatch()) {
            improved = QString("缺少分号 ';'（在 %1 之前）").arg(match.captured(1).trimmed());
        } else {
            improved = "缺少分号 ';'";
        }
    }
    
    // 规则3: "expected ',' or ';' before" 缺少逗号或分号
    else if (originalMessage.contains("expected ',' or ';' before", Qt::CaseInsensitive)) {
        improved = "缺少逗号 ',' 或分号 ';'";
    }
    
    // 规则4: "was not declared" 未声明变量
    else if (originalMessage.contains("was not declared in this scope", Qt::CaseInsensitive)) {
        QRegularExpression varRegex("'([^']+)'");
        QRegularExpressionMatch match = varRegex.match(originalMessage);
        if (match.hasMatch()) {
            QString varName = match.captured(1);
            improved = QString("变量 '%1' 未声明（检查拼写或是否忘记声明）").arg(varName);
        }
    }
    
    // 规则5: "expected unqualified-id" 语法错误
    else if (originalMessage.contains("expected unqualified-id before", Qt::CaseInsensitive)) {
        QRegularExpression beforeRegex("before\\s+(.+)");
        QRegularExpressionMatch match = beforeRegex.match(originalMessage);
        if (match.hasMatch()) {
            improved = QString("语法错误：不应该出现 %1").arg(match.captured(1).trimmed());
        }
    }
    
    // 规则6: "expected '(' before" 缺少左括号
    else if (originalMessage.contains("expected '(' before", Qt::CaseInsensitive)) {
        improved = "缺少左括号 '('（函数调用或条件语句需要括号）";
    }
    
    // 规则7: "expected ')' before" 缺少右括号
    else if (originalMessage.contains("expected ')' before", Qt::CaseInsensitive)) {
        improved = "缺少右括号 ')'（括号不匹配）";
    }
    
    // 规则8: "expected '{' before" 缺少左大括号
    else if (originalMessage.contains("expected '{' before", Qt::CaseInsensitive)) {
        improved = "缺少左大括号 '{'（函数体或代码块需要大括号）";
    }
    
    // 规则9: "expected '}' before" 缺少右大括号
    else if (originalMessage.contains("expected '}' before", Qt::CaseInsensitive)) {
        improved = "缺少右大括号 '}'（大括号不匹配）";
    }
    
    // 规则10: "expected primary-expression" 表达式错误
    else if (originalMessage.contains("expected primary-expression before", Qt::CaseInsensitive)) {
        improved = "表达式错误（可能是语法错误或缺少操作数）";
    }
    
    // 规则11: "no matching function" 函数调用错误
    else if (originalMessage.contains("no matching function for call to", Qt::CaseInsensitive)) {
        QRegularExpression funcRegex("call to '([^']+)'");
        QRegularExpressionMatch match = funcRegex.match(originalMessage);
        if (match.hasMatch()) {
            improved = QString("函数 '%1' 的参数不匹配（检查参数类型和数量）").arg(match.captured(1));
        }
    }
    
    // 规则12: "too few arguments" 参数太少
    else if (originalMessage.contains("too few arguments to function", Qt::CaseInsensitive)) {
        improved = "函数参数太少（检查函数定义需要多少个参数）";
    }
    
    // 规则13: "too many arguments" 参数太多
    else if (originalMessage.contains("too many arguments to function", Qt::CaseInsensitive)) {
        improved = "函数参数太多（检查函数定义需要多少个参数）";
    }
    
    // 规则14: "cannot convert" 类型转换错误
    else if (originalMessage.contains("cannot convert", Qt::CaseInsensitive)) {
        improved = "类型转换错误（变量类型不匹配）";
    }
    
    // 规则15: "invalid use of" 无效使用
    else if (originalMessage.contains("invalid use of", Qt::CaseInsensitive)) {
        improved = "无效使用（语法或类型错误）";
    }
    
    // 规则16: "redefinition of" 重复定义
    else if (originalMessage.contains("redefinition of", Qt::CaseInsensitive)) {
        QRegularExpression varRegex("'([^']+)'");
        QRegularExpressionMatch match = varRegex.match(originalMessage);
        if (match.hasMatch()) {
            improved = QString("'%1' 重复定义（变量或函数已经定义过）").arg(match.captured(1));
        }
    }
    
    // 规则17: "stray" 多余字符
    else if (originalMessage.contains("stray", Qt::CaseInsensitive)) {
        improved = "代码中有多余的字符（检查是否有中文符号或特殊字符）";
    }
    
    // 规则18: "expected declaration" 声明错误
    else if (originalMessage.contains("expected declaration before", Qt::CaseInsensitive)) {
        improved = "声明错误（可能是多余的分号或大括号位置错误）";
    }
    
    // 规则19: "ISO C++ forbids" 不符合C++标准
    else if (originalMessage.contains("ISO C++ forbids", Qt::CaseInsensitive)) {
        improved = "不符合C++标准（使用了非标准语法）";
    }
    
    // 规则20: "does not name a type" 类型名错误
    else if (originalMessage.contains("does not name a type", Qt::CaseInsensitive)) {
        QRegularExpression varRegex("'([^']+)'");
        QRegularExpressionMatch match = varRegex.match(originalMessage);
        if (match.hasMatch()) {
            improved = QString("'%1' 不是一个类型名（可能是拼写错误或缺少头文件）").arg(match.captured(1));
        }
    }
    
    return improved;
}

QString SyntaxChecker::translateErrorMessage(const QString &message)
{
    // 全面的编译错误汉化映射表
    static QMap<QString, QString> translations = {
        // === 语法错误 ===
        {"expected", "期望"},
        {"before", "之前"},
        {"after", "之后"},
        {"at end of input", "在输入结束处"},
        {"primary-expression", "主表达式"},
        {"unqualified-id", "标识符"},
        {"initializer", "初始化器"},
        {"declaration", "声明"},
        {"statement", "语句"},
        
        // === 符号相关 ===
        {"';'", "分号 ';'"},
        {"','", "逗号 ','"},
        {"'('", "左括号 '('"},
        {"')'", "右括号 ')'"},
        {"'{'", "左大括号 '{'"},
        {"'}'", "右大括号 '}'"},
        {"'['", "左方括号 '['"},
        {"']'", "右方括号 ']'"},
        {"'<'", "小于号 '<'"},
        {"'>'", "大于号 '>'"},
        {"'='", "等号 '='"},
        {"'*'", "星号 '*'"},
        {"'&'", "取地址符 '&'"},
        
        // === 关键字 ===
        {"'return'", "'return' 返回语句"},
        {"'if'", "'if' 条件语句"},
        {"'else'", "'else' 否则语句"},
        {"'for'", "'for' 循环语句"},
        {"'while'", "'while' 循环语句"},
        {"'do'", "'do' 循环语句"},
        {"'switch'", "'switch' 分支语句"},
        {"'case'", "'case' 分支条件"},
        {"'break'", "'break' 跳出语句"},
        {"'continue'", "'continue' 继续语句"},
        {"'int'", "'int' 整数类型"},
        {"'char'", "'char' 字符类型"},
        {"'float'", "'float' 浮点类型"},
        {"'double'", "'double' 双精度类型"},
        {"'void'", "'void' 空类型"},
        {"'bool'", "'bool' 布尔类型"},
        {"'string'", "'string' 字符串类型"},
        {"'const'", "'const' 常量"},
        {"'static'", "'static' 静态"},
        {"'class'", "'class' 类"},
        {"'struct'", "'struct' 结构体"},
        {"'namespace'", "'namespace' 命名空间"},
        {"'using'", "'using' 使用声明"},
        {"'template'", "'template' 模板"},
        {"'typename'", "'typename' 类型名"},
        
        // === 标准库 ===
        {"'cout'", "'cout' 输出流"},
        {"'cin'", "'cin' 输入流"},
        {"'endl'", "'endl' 换行"},
        {"'std'", "'std' 标准命名空间"},
        {"'vector'", "'vector' 向量容器"},
        {"'string'", "'string' 字符串"},
        {"'main'", "'main' 主函数"},
        
        // === 错误类型 ===
        {"error:", "错误:"},
        {"warning:", "警告:"},
        {"note:", "提示:"},
        
        // === 常见错误信息 ===
        {"was not declared in this scope", "未在此作用域中声明"},
        {"has not been declared", "未被声明"},
        {"undeclared identifier", "未声明的标识符"},
        {"does not name a type", "不是一个类型名"},
        {"is not a member of", "不是...的成员"},
        {"redefinition of", "重复定义"},
        {"conflicting declaration", "声明冲突"},
        {"previous declaration", "之前的声明"},
        {"invalid use of", "无效使用"},
        {"cannot convert", "无法转换"},
        {"incompatible types", "类型不兼容"},
        {"no matching function", "没有匹配的函数"},
        {"no matching constructor", "没有匹配的构造函数"},
        {"too few arguments", "参数太少"},
        {"too many arguments", "参数太多"},
        {"invalid conversion", "无效转换"},
        {"invalid operands", "无效操作数"},
        {"lvalue required", "需要左值"},
        {"assignment of read-only", "对只读变量赋值"},
        {"increment of read-only", "对只读变量自增"},
        {"decrement of read-only", "对只读变量自减"},
        
        // === 作用域和访问 ===
        {"in this scope", "在此作用域中"},
        {"out of scope", "超出作用域"},
        {"private", "私有"},
        {"protected", "受保护"},
        {"public", "公有"},
        {"within this context", "在此上下文中"},
        
        // === 类型相关 ===
        {"type", "类型"},
        {"variable", "变量"},
        {"function", "函数"},
        {"class", "类"},
        {"struct", "结构体"},
        {"array", "数组"},
        {"pointer", "指针"},
        {"reference", "引用"},
        
        // === 其他常见错误 ===
        {"stray", "多余的"},
        {"in program", "在程序中"},
        {"ISO C++ forbids", "ISO C++ 标准禁止"},
        {"deprecated", "已弃用"},
        {"ambiguous", "有歧义的"},
        {"overflow", "溢出"},
        {"underflow", "下溢"},
        {"division by zero", "除以零"},
        {"subscript", "下标"},
        {"out of range", "超出范围"},
        {"null pointer", "空指针"},
        {"memory", "内存"},
        {"syntax error", "语法错误"},
        {"parse error", "解析错误"},
        {"undefined reference", "未定义的引用"},
        {"multiple definition", "多重定义"},
        {"first defined here", "首次定义于此"},
        
        // === 模板相关 ===
        {"template argument", "模板参数"},
        {"template parameter", "模板形参"},
        {"instantiation", "实例化"},
        {"specialization", "特化"},
        
        // === 链接错误 ===
        {"undefined reference to", "未定义的引用"},
        {"multiple definition of", "多重定义"},
        {"ld returned", "链接器返回"},
        {"collect2: error:", "collect2: 错误:"}
    };
    
    QString result = message;
    
    // 按照从长到短的顺序替换，避免部分匹配
    QList<QPair<int, QString>> sortedKeys;
    for (auto it = translations.constBegin(); it != translations.constEnd(); ++it) {
        sortedKeys.append(qMakePair(it.key().length(), it.key()));
    }
    std::sort(sortedKeys.begin(), sortedKeys.end(), 
              [](const QPair<int, QString> &a, const QPair<int, QString> &b) {
                  return a.first > b.first;
              });
    
    for (const auto &pair : sortedKeys) {
        const QString &key = pair.second;
        const QString &value = translations[key];
        result.replace(key, value, Qt::CaseInsensitive);
    }
    
    return result;
}

QVector<SyntaxError> SyntaxChecker::parseCompilerOutput(const QString &output)
{
    QVector<SyntaxError> errors;
    QMap<int, int> lineErrorCount;  // 统计每行的错误数
    
    // 解析GCC/MinGW错误格式：file:line:column: error/warning: message
    // 示例: temp.cpp:5:10: error: expected ';' before 'return'
    QRegularExpression regex(R"(.*:(\d+):(\d+):\s+(error|warning):\s+(.+))");
    
    QStringList lines = output.split('\n');
    qDebug() << "[SyntaxChecker] Parsing" << lines.size() << "lines of output";
    
    for (const QString &line : lines) {
        QString trimmedLine = line.trimmed();
        if (trimmedLine.isEmpty()) continue;
        
        QRegularExpressionMatch match = regex.match(trimmedLine);
        if (match.hasMatch()) {
            SyntaxError error;
            error.line = match.captured(1).toInt();
            error.column = match.captured(2).toInt();
            error.type = match.captured(3);
            QString originalMessage = match.captured(4).trimmed();
            
            // 第一步：智能改进错误信息（提供更准确的提示）
            error.message = improveErrorMessage(originalMessage, error.line);
            
            // 第二步：如果没有改进，则进行全面翻译
            if (error.message == originalMessage) {
                error.message = translateErrorMessage(originalMessage);
            }
            
            // 翻译错误类型
            if (error.type == "error") {
                error.type = "错误";
            } else if (error.type == "warning") {
                error.type = "警告";
            } else if (error.type == "note") {
                error.type = "提示";
            }
            
            // 统计该行的错误数
            lineErrorCount[error.line]++;
            
            qDebug() << "[SyntaxChecker] Parsed" << error.type 
                     << "at line" << error.line 
                     << "col" << error.column 
                     << ":" << error.message;
            errors.append(error);
        }
    }
    
    // 输出每行的错误统计
    for (auto it = lineErrorCount.begin(); it != lineErrorCount.end(); ++it) {
        qDebug() << "[SyntaxChecker] Line" << it.key() << "has" << it.value() << "error(s)";
    }
    
    qDebug() << "[SyntaxChecker] Total errors parsed:" << errors.size();
    return errors;
}
