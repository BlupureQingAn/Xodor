#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSplitter>
#include <QStackedWidget>
#include "QuestionPanel.h"
#include "CodeEditor.h"
#include "AIAssistantPanel.h"
#include "AIJudgeProgressDialog.h"
#include "QuestionBankPanel.h"
#include "PracticeWidget.h"
#include "../core/QuestionBank.h"
#include "../core/CompilerRunner.h"
#include "../core/CodeVersionManager.h"
#include "../ai/OllamaClient.h"
#include "../utils/AIConnectionChecker.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    
protected:
    void closeEvent(QCloseEvent *event) override;
    
private slots:
    // 菜单操作
    void onImportQuestionBank();
    void onRefreshQuestionBank();
    void onReloadQuestionBank();
    void onManageQuestionBanks();
    void onClearQuestionBank();
    void onViewOriginalQuestion();
    void onGenerateExam();
    void onManageMockExams();
    void onShowHistory();
    void onShowWrongBook();
    void onShowCodeVersionHistory();
    void onFixTestCases();  // 修复测试用例
    void onBatchFixTestCases();  // 批量修复测试用例
    void onToggleAIAssistant();
    void onSwitchToQuestionList();  // 切换到题库列表
    void onSwitchToPracticeMode();  // 切换到刷题模式
    void onShowSettings();
    void onAbout();
    void onInsertTemplate(const QString &templateName);
    
    // 撤销/重做
    void onUndo();
    void onRedo();
    void onShowOperationHistory();
    
    // 题目操作
    void onAIJudgeRequested();  // AI判题
    void onAIJudgeCompleted(bool passed, const QString &comment, const QVector<int> &failedTestCases);
    void onAIJudgeError(const QString &error);
    void onNextQuestion();
    void onPreviousQuestion();
    void onQuestionSelectedFromList(int index);
    void onQuestionFileSelected(const QString &filePath, const Question &question);
    void onBankSelectedFromPanel(const QString &bankPath);
    void onRefreshCurrentBank();  // 刷新当前题库
    
    // AI操作
    void onRequestAnalysis();
    void onAnalysisReady(const QString &analysis);
    void onAIError(const QString &error);
    
    // 错误处理
    void onErrorClicked(int line, int column);
    void onSyntaxErrorsFound(const QVector<class SyntaxError> &errors);
    
    // 题库操作
    void onQuestionsLoaded(int count);
    void onDeleteQuestions(const QVector<int> &indices);
    
private:
    void setupUI();
    void setupMenuBar();
    void setupToolBar();
    void setupConnections();
    void loadConfiguration();
    void loadCurrentQuestion();
    void loadSavedCode(const QString &questionId);
    QString loadSavedCodeForQuestion(const QString &questionId);
    void saveQuestionBank();
    QString generateDefaultCode(const Question &question);
    void showTestResults(const QVector<TestResult> &results);
    void loadLastSession();
    void restoreWindowState();
    void applyModernStyle();
    void importQuestionsFromPath(const QString &path);
    void checkAIConnection();
    void showAIConnectionStatus(const AIConnectionStatus &status);
    void showAIConfigDialog(const AIConnectionStatus &status);
    
    // UI组件
    QSplitter *m_mainSplitter;
    QuestionPanel *m_questionPanel;
    CodeEditor *m_codeEditor;
    AIAssistantPanel *m_aiAssistantPanel;
    QuestionBankPanel *m_questionBankPanel;
    PracticeWidget *m_practiceWidget;
    QWidget *m_normalModeWidget;
    QStackedWidget *m_stackedWidget;
    QDockWidget *m_aiAssistantDock;
    class ErrorListWidget *m_errorListWidget;
    
    // 核心组件
    QuestionBank *m_questionBank;
    OllamaClient *m_ollamaClient;
    CompilerRunner *m_compilerRunner;
    CodeVersionManager *m_versionManager;
    class AIJudge *m_aiJudge;
    
    // UI组件
    AIJudgeProgressDialog *m_aiJudgeProgressDialog;
    
    // 状态
    int m_currentQuestionIndex;
    QString m_lastImportPath;
    QString m_currentBankPath;  // 当前题库路径
    AIConnectionStatus m_lastAIStatus;  // 最后一次AI连接状态
};

#endif // MAINWINDOW_H
