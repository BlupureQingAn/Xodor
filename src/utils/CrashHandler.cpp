#include "CrashHandler.h"
#include <QApplication>
#include <QMessageBox>
#include <QDateTime>
#include <QFile>
#include <QTextStream>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>

void CrashHandler::install()
{
    // 安装Qt消息处理器
    qInstallMessageHandler(CrashHandler::messageHandler);
    
    qDebug() << "CrashHandler installed";
}

void CrashHandler::messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QString formattedMsg = qFormatLogMessage(type, context, msg);
    
    // 输出到控制台
    fprintf(stderr, "%s\n", formattedMsg.toLocal8Bit().constData());
    fflush(stderr);
    
    // 保存到日志文件
    QString logPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/crash.log";
    QFile logFile(logPath);
    if (logFile.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream stream(&logFile);
        stream << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << " ";
        stream << formattedMsg << "\n";
        logFile.close();
    }
    
    // 如果是严重错误，显示对话框
    if (type == QtCriticalMsg || type == QtFatalMsg) {
        QString reason;
        switch (type) {
            case QtCriticalMsg:
                reason = "程序遇到严重错误";
                break;
            case QtFatalMsg:
                reason = "程序遇到致命错误";
                break;
            default:
                reason = "程序错误";
                break;
        }
        
        QString details = QString("文件：%1\n"
                                 "行号：%2\n"
                                 "函数：%3\n"
                                 "错误：%4")
                         .arg(context.file ? context.file : "未知")
                         .arg(context.line)
                         .arg(context.function ? context.function : "未知")
                         .arg(msg);
        
        saveCrashLog(reason, details);
        
        // 如果是致命错误，显示对话框后退出
        if (type == QtFatalMsg) {
            showCrashDialog(reason, details);
            abort();
        }
    }
}

void CrashHandler::showCrashDialog(const QString &reason, const QString &details)
{
    QMessageBox msgBox;
    msgBox.setWindowTitle("程序崩溃");
    msgBox.setIcon(QMessageBox::Critical);
    
    QString text = QString("😢 很抱歉，%1\n\n"
                          "程序将自动保存当前状态并退出。\n\n"
                          "崩溃日志已保存到：\n"
                          "%2/crash.log")
                  .arg(reason)
                  .arg(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    
    msgBox.setText(text);
    
    if (!details.isEmpty()) {
        msgBox.setDetailedText(details);
    }
    
    msgBox.setInformativeText("您可以尝试：\n"
                             "1. 重启程序\n"
                             "2. 检查是否有未保存的工作\n"
                             "3. 查看崩溃日志了解详情\n"
                             "4. 联系技术支持");
    
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
            padding: 10px 20px;
            font-weight: 500;
            min-width: 80px;
        }
        QPushButton:hover {
            background-color: #880000;
        }
        QTextEdit {
            background-color: #1e1e1e;
            color: #e8e8e8;
            border: 1px solid #3a3a3a;
        }
    )");
    
    msgBox.exec();
}

QString CrashHandler::getStackTrace()
{
    // 简单的堆栈跟踪（Windows上需要更复杂的实现）
    return "堆栈跟踪功能需要调试符号支持";
}

void CrashHandler::saveCrashLog(const QString &reason, const QString &details)
{
    QString logPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(logPath);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    
    QString crashLogPath = logPath + "/crash_" + 
                          QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss") + ".log";
    
    QFile file(crashLogPath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        stream << "=== 程序崩溃报告 ===\n";
        stream << "时间：" << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "\n";
        stream << "原因：" << reason << "\n";
        stream << "\n=== 详细信息 ===\n";
        stream << details << "\n";
        stream << "\n=== 系统信息 ===\n";
        stream << "Qt版本：" << QT_VERSION_STR << "\n";
        stream << "应用版本：1.7.2\n";
        file.close();
        
        qDebug() << "Crash log saved to:" << crashLogPath;
    }
}
