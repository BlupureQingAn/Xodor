#ifndef CODEEDITOR_H
#define CODEEDITOR_H

#include <QWidget>
#include <Qsci/qsciscintilla.h>
#include "../core/AutoSaver.h"
#include "../utils/SyntaxChecker.h"

class OllamaClient;

class CodeEditor : public QWidget
{
    Q_OBJECT
public:
    explicit CodeEditor(QWidget *parent = nullptr);
    
    void setCode(const QString &code);
    QString code() const;
    void setQuestionId(const QString &id);
    QString getQuestionId() const { return m_currentQuestionId; }
    
    // 获取AutoSaver（用于设置版本管理器）
    AutoSaver* autoSaver() const { return m_autoSaver; }
    
    // 新增功能
    void setAIClient(OllamaClient *client) { m_aiClient = client; }
    void setCompiler(const QString &compiler);
    void checkSyntax();
    void enableSyntaxCheck(bool enabled);
    void setCursorPosition(int line, int col);
    void ensureLineVisible(int line);
    void forceSave();  // 强制立即保存
    
signals:
    void codeChanged(const QString &code);
    void requestAnalysis();
    void syntaxErrorsFound(const QVector<SyntaxError> &errors);
    
private slots:
    void onTextChanged();
    void onSyntaxErrors(const QVector<SyntaxError> &errors);
    
private:
    void setupEditor();
    void setupSyntaxChecker();
    void setupErrorIndicators();
    bool handleBracketCompletion(QKeyEvent *event);
    bool shouldCompleteAngleBracket();  // 判断是否应该补全尖括号
    bool handleDotCompletion();  // 处理点号补全（STL容器成员）
    bool handleArrowCompletion();  // 处理箭头操作符补全（迭代器等）
    bool handleScopeCompletion();  // 处理作用域操作符补全（std::等）
    void handleKeywordCompletion();  // 处理关键字和常用函数补全
    void insertMatchingBracket(const QString &openBracket, const QString &closeBracket);
    void markErrors(const QVector<SyntaxError> &errors);
    void clearErrorMarkers();
    void toggleComment();  // 切换注释
    
protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    
private:
    QsciScintilla *m_editor;
    AutoSaver *m_autoSaver;
    QString m_currentQuestionId;
    SyntaxChecker *m_syntaxChecker;
    OllamaClient *m_aiClient;
    
    // 错误标记
    int m_errorIndicator;
    int m_warningIndicator;
};

#endif // CODEEDITOR_H
