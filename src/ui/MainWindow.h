#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSplitter>
#include <QStackedWidget>
#include "QuestionPanel.h"
#include "CodeEditor.h"
#include "AIAssistantPanel.h"
#include "QuestionListWidget.h"
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
    void onToggleAIAssistant();
    void onSwitchToQuestionList();  // 切换到题库列表
    void onSwitchToPracticeMode();  // 切换到刷题模式
    void onShowSettings();
    void onAbout();
    void onInsertTemplate(const QString &templateName);
    
    // 题目操作
    void onRunTests();
    void onNextQuestion();
    void onPreviousQuestion();
    void onQuestionSelectedFromList(int index);
    
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
    void saveQuestionBank();
    QString generateDefaultCode(const Question &question);
    void showTestResults(const QVector<TestResult> &results);
    void loadLastSession();
    void restoreWindowState();
    void applyModernStyle();
    void importQuestionsFromPath(const QString &path);
    void checkAIConnection();
    void showAIConnectionStatus(const AIConnectionStatus &status);
    void checkAndSelectModel();
    
    // UI组件
    QSplitter *m_mainSplitter;
    QuestionPanel *m_questionPanel;
    CodeEditor *m_codeEditor;
    AIAssistantPanel *m_aiAssistantPanel;
    QuestionListWidget *m_questionListWidget;
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
    
    // 状态
    int m_currentQuestionIndex;
    QString m_lastImportPath;
};

#endif // MAINWINDOW_H
