#ifndef CRASHHANDLER_H
#define CRASHHANDLER_H

#include <QString>
#include <QWidget>

// 全局崩溃处理器
class CrashHandler
{
public:
    static void install();
    static void showCrashDialog(const QString &reason, const QString &details = QString());
    
private:
    static void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg);
    static QString getStackTrace();
    static void saveCrashLog(const QString &reason, const QString &details);
};

#endif // CRASHHANDLER_H
