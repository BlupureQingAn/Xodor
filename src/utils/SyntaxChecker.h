#ifndef SYNTAXCHECKER_H
#define SYNTAXCHECKER_H

#include <QObject>
#include <QTimer>
#include <QProcess>
#include <QVector>

struct SyntaxError {
    int line;
    int column;
    QString message;
    QString type;  // error, warning
};

class SyntaxChecker : public QObject
{
    Q_OBJECT
public:
    explicit SyntaxChecker(QObject *parent = nullptr);
    
    void checkCode(const QString &code, const QString &compiler);
    void setCheckDelay(int ms) { m_checkTimer->setInterval(ms); }
    
signals:
    void errorsFound(const QVector<SyntaxError> &errors);
    void checkFinished();
    
private slots:
    void performCheck();
    void onProcessFinished(int exitCode, QProcess::ExitStatus status);
    
private:
    QVector<SyntaxError> parseCompilerOutput(const QString &output);
    QString improveErrorMessage(const QString &originalMessage, int line);
    QString translateErrorMessage(const QString &message);
    
    QTimer *m_checkTimer;
    QProcess *m_process;
    QString m_pendingCode;
    QString m_lastCheckedCode;  // 缓存上次检查的代码
    QString m_compiler;
    QString m_tempFilePath;
};

#endif // SYNTAXCHECKER_H
