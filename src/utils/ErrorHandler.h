#ifndef ERRORHANDLER_H
#define ERRORHANDLER_H

#include <QString>
#include <QWidget>

enum class ErrorType {
    Compile,
    Network,
    FileIO,
    Configuration,
    Runtime,
    Unknown
};

enum class ErrorSeverity {
    Info,
    Warning,
    Error,
    Critical
};

class ErrorHandler
{
public:
    static void handleError(
        QWidget *parent,
        ErrorType type,
        const QString &message,
        const QString &details = QString(),
        ErrorSeverity severity = ErrorSeverity::Error
    );
    
    static void handleCompileError(QWidget *parent, const QString &error);
    static void handleNetworkError(QWidget *parent, const QString &error);
    static void handleFileError(QWidget *parent, const QString &filePath, const QString &error);
    static void handleConfigError(QWidget *parent, const QString &error);
    
    static QString formatErrorMessage(ErrorType type, const QString &message);
    static QString getErrorIcon(ErrorType type);
    static QString getSuggestion(ErrorType type, const QString &error);
    
private:
    static void showErrorDialog(
        QWidget *parent,
        const QString &title,
        const QString &message,
        const QString &details,
        ErrorSeverity severity
    );
    
    static void logError(ErrorType type, const QString &message);
};

#endif // ERRORHANDLER_H
