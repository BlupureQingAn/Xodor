#ifndef ENHANCEDCODEEDITOR_H
#define ENHANCEDCODEEDITOR_H

#include <QWidget>
#include <QTimer>
#include <QList>
#include <Qsci/qsciscintilla.h>
#include "../core/AutoSaver.h"

// 错误信息结构
struct CodeError {
    int line;           // 行号（从0开始）
    int column;         // 列号
    QString message;    // 错误消息
    QString type;       // 错误类型：error, warning, info
    
    CodeError(int l = 0, int c = 0, const QString &msg = "", const QString &t = "error")
        : line(l), column(c), message(msg), type(t) {}
};

class EnhancedCodeEditor : public QWidget
{
    Q_OBJECT
public:
    explicit EnhancedCodeEditor(QWidget *parent = nullptr);
    
    void setCode(const QString &code);
    QString code() const;
    void setQuestionId(const QString &id);
    
    // 获取AutoSaver（用于设置版本管理器）
    AutoSaver* autoSaver() const { return m_autoSaver; }
    
    // 错误管理
    void clearErrors();
    void addError(const CodeError &error);
    QList<CodeError> getErrors() const { return m_errors; }
    
    // 括号自动补全开关
    void setAutoBracketEnabled(bool enabled) { m_autoBracketEnabled = enabled; }
    bool isAutoBracketEnabled() const { return m_autoBracketEnabled; }
    
    // 实时语法检查开关
    void setSyntaxCheckEnabled(bool enabled);
    bool isSyntaxCheckEnabled() const { return m_syntaxCheckEnabled; }
    
signals:
    void codeChanged(const QString &code);
    void requestAnalysis();
    void errorsChanged(const QList<CodeError> &errors);
    void requestAIFix(const QString &code, const QList<CodeError> &errors);
    
private slots:
    void onTextChanged();
    void onCharAdded(int ch);
    void performSyntaxCheck();
    
private:
    void setupEditor();
    void setupErrorMarkers();
    void updateErrorMarkers();
    void checkBracketBalance();
    QString getMatchingBracket(QChar openBracket);
    bool shouldAutoComplete(QChar ch);
    
    QsciScintilla *m_editor;
    AutoSaver *m_autoSaver;
    QString m_currentQuestionId;
    
    // 错误管理
    QList<CodeError> m_errors;
    QTimer *m_syntaxCheckTimer;
    
    // 功能开关
    bool m_autoBracketEnabled;
    bool m_syntaxCheckEnabled;
    
    // 错误标记
    static const int ERROR_MARKER = 1;
    static const int WARNING_MARKER = 2;
};

#endif // ENHANCEDCODEEDITOR_H
