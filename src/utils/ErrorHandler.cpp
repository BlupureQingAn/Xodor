#include "ErrorHandler.h"
#include <QMessageBox>
#include <QFile>
#include <QDir>
#include <QDateTime>
#include <QTextStream>

void ErrorHandler::handleError(
    QWidget *parent,
    ErrorType type,
    const QString &message,
    const QString &details,
    ErrorSeverity severity)
{
    QString title = formatErrorMessage(type, "");
    QString fullMessage = message;
    
    // 添加建议
    QString suggestion = getSuggestion(type, message);
    if (!suggestion.isEmpty()) {
        fullMessage += "\n\n💡 建议：\n" + suggestion;
    }
    
    // 记录错误日志
    logError(type, message + (details.isEmpty() ? "" : "\n详情：" + details));
    
    // 显示错误对话框
    showErrorDialog(parent, title, fullMessage, details, severity);
}

void ErrorHandler::handleCompileError(QWidget *parent, const QString &error)
{
    QString message = "代码编译失败";
    QString details = error;
    
    // 解析常见编译错误
    if (error.contains("error: expected")) {
        message += "\n\n可能的原因：语法错误，缺少分号、括号或其他符号";
    } else if (error.contains("undefined reference")) {
        message += "\n\n可能的原因：函数未定义或链接错误";
    } else if (error.contains("no matching function")) {
        message += "\n\n可能的原因：函数调用参数不匹配";
    }
    
    handleError(parent, ErrorType::Compile, message, details, ErrorSeverity::Error);
}

void ErrorHandler::handleNetworkError(QWidget *parent, const QString &error)
{
    QString message = "网络请求失败";
    
    if (error.contains("Connection refused") || error.contains("连接被拒绝")) {
        message += "\n\n可能的原因：\n"
                  "• Ollama 服务未启动\n"
                  "• 服务地址配置错误\n"
                  "• 防火墙阻止连接";
    } else if (error.contains("timeout") || error.contains("超时")) {
        message += "\n\n可能的原因：\n"
                  "• 网络连接不稳定\n"
                  "• 服务响应缓慢\n"
                  "• 请求超时";
    }
    
    handleError(parent, ErrorType::Network, message, error, ErrorSeverity::Warning);
}

void ErrorHandler::handleFileError(QWidget *parent, const QString &filePath, const QString &error)
{
    QString message = QString("文件操作失败：%1").arg(filePath);
    
    if (error.contains("Permission denied") || error.contains("权限")) {
        message += "\n\n可能的原因：\n"
                  "• 文件被其他程序占用\n"
                  "• 没有读写权限\n"
                  "• 文件夹不存在";
    } else if (error.contains("No such file") || error.contains("不存在")) {
        message += "\n\n可能的原因：\n"
                  "• 文件不存在\n"
                  "• 路径错误\n"
                  "• 文件已被删除";
    }
    
    handleError(parent, ErrorType::FileIO, message, error, ErrorSeverity::Error);
}

void ErrorHandler::handleConfigError(QWidget *parent, const QString &error)
{
    QString message = "配置错误";
    handleError(parent, ErrorType::Configuration, message, error, ErrorSeverity::Warning);
}

QString ErrorHandler::formatErrorMessage(ErrorType type, const QString &message)
{
    QString prefix;
    switch (type) {
        case ErrorType::Compile:
            prefix = "编译错误";
            break;
        case ErrorType::Network:
            prefix = "网络错误";
            break;
        case ErrorType::FileIO:
            prefix = "文件错误";
            break;
        case ErrorType::Configuration:
            prefix = "配置错误";
            break;
        case ErrorType::Runtime:
            prefix = "运行时错误";
            break;
        default:
            prefix = "错误";
            break;
    }
    
    return message.isEmpty() ? prefix : prefix + ": " + message;
}

QString ErrorHandler::getErrorIcon(ErrorType type)
{
    switch (type) {
        case ErrorType::Compile:
            return "🔧";
        case ErrorType::Network:
            return "🌐";
        case ErrorType::FileIO:
            return "📁";
        case ErrorType::Configuration:
            return "⚙️";
        case ErrorType::Runtime:
            return "⚠️";
        default:
            return "❌";
    }
}

QString ErrorHandler::getSuggestion(ErrorType type, const QString &error)
{
    switch (type) {
        case ErrorType::Compile:
            return "• 检查代码语法是否正确\n"
                   "• 确保所有变量都已声明\n"
                   "• 检查括号、分号是否匹配";
            
        case ErrorType::Network:
            if (error.contains("Ollama") || error.contains("ollama")) {
                return "• 确保 Ollama 服务正在运行（ollama serve）\n"
                       "• 检查配置文件中的服务地址\n"
                       "• 尝试重启 Ollama 服务";
            }
            return "• 检查网络连接\n"
                   "• 确认服务地址正确\n"
                   "• 稍后重试";
            
        case ErrorType::FileIO:
            return "• 检查文件路径是否正确\n"
                   "• 确保有足够的权限\n"
                   "• 检查磁盘空间";
            
        case ErrorType::Configuration:
            return "• 检查配置文件格式\n"
                   "• 恢复默认配置\n"
                   "• 查看文档说明";
            
        default:
            return "";
    }
}

void ErrorHandler::showErrorDialog(
    QWidget *parent,
    const QString &title,
    const QString &message,
    const QString &details,
    ErrorSeverity severity)
{
    QMessageBox msgBox(parent);
    msgBox.setWindowTitle(title);
    msgBox.setText(message);
    
    if (!details.isEmpty()) {
        msgBox.setDetailedText(details);
    }
    
    // 设置图标
    switch (severity) {
        case ErrorSeverity::Info:
            msgBox.setIcon(QMessageBox::Information);
            break;
        case ErrorSeverity::Warning:
            msgBox.setIcon(QMessageBox::Warning);
            break;
        case ErrorSeverity::Error:
            msgBox.setIcon(QMessageBox::Critical);
            break;
        case ErrorSeverity::Critical:
            msgBox.setIcon(QMessageBox::Critical);
            break;
    }
    
    // 应用样式
    msgBox.setStyleSheet(R"(
        QMessageBox {
            background-color: #242424;
        }
        QMessageBox QLabel {
            color: #e8e8e8;
            font-size: 10pt;
        }
        QPushButton {
            background-color: #660000;
            color: white;
            border: none;
            border-radius: 8px;
            padding: 8px 20px;
            font-weight: 500;
            min-width: 80px;
        }
        QPushButton:hover {
            background-color: #880000;
        }
        QPushButton:pressed {
            background-color: #440000;
        }
    )");
    
    msgBox.exec();
}

void ErrorHandler::logError(ErrorType type, const QString &message)
{
    QDir dir("data/logs");
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    
    QString logFile = "data/logs/error.log";
    QFile file(logFile);
    
    if (file.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&file);
        out << QDateTime::currentDateTime().toString(Qt::ISODate) << " | ";
        out << formatErrorMessage(type, "") << " | ";
        out << message << "\n";
        file.close();
    }
}
