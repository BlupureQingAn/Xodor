#include "MainWindow.h"
#include "SmartImportDialog.h"
#include "QuestionBankManagerDialog.h"
#include "ExamGeneratorDialog.h"
#include "MockExamManagerDialog.h"
#include "HistoryWidget.h"
#include "QuestionBankPanel.h"
#include "WrongQuestionWidget.h"
#include "PracticeWidget.h"
#include "SettingsDialog.h"
#include "OriginalQuestionDialog.h"
#include "CodeVersionDialog.h"
#include "ErrorListWidget.h"
#include "TestCaseFixerDialog.h"
#include "StyleManager.h"
#include "../core/QuestionBankManager.h"
#include "../ai/AIJudge.h"
#include "../utils/AIConnectionChecker.h"
#include "../utils/OperationHistory.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTimer>
#include <QInputDialog>
#include <QFileDialog>
#include <QFileInfo>
#include <QListWidget>
#include "../ai/QuestionParser.h"
#include "../core/WrongQuestionBook.h"
#include "../core/ProgressManager.h"
#include "../utils/ConfigManager.h"
#include "../utils/CompilerDetector.h"
#include "../utils/SessionManager.h"
#include "../utils/CodeTemplateManager.h"
#include "../utils/ErrorHandler.h"
#include <QMenuBar>
#include <QAction>
#include <QMessageBox>
#include <QDir>
#include <QTabWidget>
#include <algorithm>
#include <QFileDialog>
#include <QStatusBar>
#include <QJsonDocument>
#include <QJsonArray>
#include <QRegularExpression>
#include <QToolBar>
#include <QDockWidget>
#include <QInputDialog>
#include <QCloseEvent>
#include <functional>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_currentQuestionIndex(-1)
    , m_aiJudgeProgressDialog(nullptr)
{
    setupUI();
    setupMenuBar();
    setupConnections();
    loadConfiguration();
    
    resize(1400, 800);
    setWindowTitle("代码刷题系统");
    
    // 应用现代化样式
    applyModernStyle();
    
    // 恢复窗口状态
    restoreWindowState();
    
    // 自动加载上次的题库
    loadLastSession();
    
    // 启动时检查AI连接（延迟500ms，让界面先显示）
    QTimer::singleShot(500, this, &MainWindow::checkAIConnection);
}

void MainWindow::applyModernStyle()
{
    // 应用主窗口样式
    setStyleSheet(StyleManager::getMainWindowStyle());
    
    // 应用菜单栏样式
    menuBar()->setStyleSheet(StyleManager::getMenuBarStyle());
    
    // 应用工具栏样式
    for (QToolBar *toolbar : findChildren<QToolBar*>()) {
        toolbar->setStyleSheet(StyleManager::getToolBarStyle());
    }
    
    // 应用题目面板样式
    m_questionPanel->setStyleSheet(StyleManager::getQuestionPanelStyle());
    
    // 应用代码编辑器样式
    m_codeEditor->setStyleSheet(StyleManager::getCodeEditorStyle());
    
    // 应用AI导师面板样式
    
    // 应用题库面板样式
    m_questionBankPanel->setStyleSheet(StyleManager::getQuestionBankPanelStyle());
    
    // 应用状态栏样式
    statusBar()->setStyleSheet(
        "QStatusBar { background-color: #242424; color: #e8e8e8; "
        "border-top: 1px solid #3a3a3a; padding: 4px 8px; }"
    );
}

void MainWindow::setupUI()
{
    // 初始化题库和AI服务（必须先初始化）
    m_questionBank = new QuestionBank(this);
    m_ollamaClient = new OllamaClient(this);
    m_compilerRunner = new CompilerRunner(this);
    m_versionManager = new CodeVersionManager(this);
    m_aiJudge = new AIJudge(m_ollamaClient, this);
    
    // 创建堆叠窗口用于切换视图
    m_stackedWidget = new QStackedWidget(this);
    
    // === 正常模式（原有的编辑界面） ===
    m_normalModeWidget = new QWidget(this);
    QVBoxLayout *normalLayout = new QVBoxLayout(m_normalModeWidget);
    normalLayout->setContentsMargins(0, 0, 0, 0);
    
    // 创建主分割器
    m_mainSplitter = new QSplitter(Qt::Horizontal, m_normalModeWidget);
    
    m_questionPanel = new QuestionPanel(m_normalModeWidget);
    
    // 创建代码编辑器区域（包含编辑器和底部错误列表）
    QWidget *editorArea = new QWidget(m_normalModeWidget);
    QVBoxLayout *editorLayout = new QVBoxLayout(editorArea);
    editorLayout->setContentsMargins(0, 0, 0, 0);
    editorLayout->setSpacing(0);
    
    m_codeEditor = new CodeEditor(editorArea);
    
    // 设置版本管理器到AutoSaver
    m_codeEditor->autoSaver()->setVersionManager(m_versionManager);
    
    // 创建错误列表面板（可折叠）
    m_errorListWidget = new ErrorListWidget(editorArea);
    m_errorListWidget->setAIClient(m_ollamaClient);
    m_errorListWidget->setMaximumHeight(200);  // 限制最大高度
    m_errorListWidget->setVisible(false);  // 默认隐藏
    
    editorLayout->addWidget(m_codeEditor);
    editorLayout->addWidget(m_errorListWidget);
    
    m_mainSplitter->addWidget(m_questionPanel);
    m_mainSplitter->addWidget(editorArea);
    
    // 设置初始比例：题目面板 2，编辑器区域 3
    m_mainSplitter->setStretchFactor(0, 2);
    m_mainSplitter->setStretchFactor(1, 3);
    
    // 美化分隔条样式
    m_mainSplitter->setHandleWidth(1);
    m_mainSplitter->setStyleSheet(
        "QSplitter::handle {"
        "    background-color: #3a3a3a;"
        "}"
        "QSplitter::handle:hover {"
        "    background-color: #660000;"
        "}"
    );
    
    normalLayout->addWidget(m_mainSplitter);
    
    // === 题库列表 ===
    m_practiceWidget = new PracticeWidget(m_questionBank, this);
    
    // 添加到堆叠窗口
    m_stackedWidget->addWidget(m_normalModeWidget);  // index 0
    m_stackedWidget->addWidget(m_practiceWidget);    // index 1
    
    setCentralWidget(m_stackedWidget);
    
    // 创建题库面板侧边栏
    m_questionBankPanel = new QuestionBankPanel(this);
    QDockWidget *questionListDock = new QDockWidget("题目列表", this);
    questionListDock->setWidget(m_questionBankPanel);
    questionListDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    addDockWidget(Qt::LeftDockWidgetArea, questionListDock);
    
    // 创建AI导师面板（可停靠，默认显示）
    m_aiAssistantPanel = new AIAssistantPanel(m_ollamaClient, this);
    m_aiAssistantDock = new QDockWidget("🤖 AI 导师", this);
    m_aiAssistantDock->setWidget(m_aiAssistantPanel);
    m_aiAssistantDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    addDockWidget(Qt::RightDockWidgetArea, m_aiAssistantDock);
    // 默认显示AI导师面板
    
    // 创建工具栏
    setupToolBar();
}

void MainWindow::setupToolBar()
{
    QToolBar *toolBar = addToolBar("主工具栏");
    toolBar->setMovable(false);
    toolBar->setIconSize(QSize(24, 24));
    
    // 视图切换
    QAction *practiceAction = toolBar->addAction("📊 题库列表");
    practiceAction->setShortcut(QKeySequence("Ctrl+P"));
    practiceAction->setToolTip("切换到题库列表 (Ctrl+P)");
    practiceAction->setStatusTip("查看题库列表，选择题目");
    connect(practiceAction, &QAction::triggered, this, &MainWindow::onSwitchToQuestionList);
    
    QAction *normalAction = toolBar->addAction("✏️ 刷题模式");
    normalAction->setShortcut(QKeySequence("Ctrl+E"));
    normalAction->setToolTip("切换到刷题模式 (Ctrl+E)");
    normalAction->setStatusTip("进入刷题模式，编写代码");
    connect(normalAction, &QAction::triggered, this, &MainWindow::onSwitchToPracticeMode);
    
    toolBar->addSeparator();
    
    // 导入题库
    QAction *importAction = toolBar->addAction("🤖 AI导入题库");
    importAction->setToolTip("AI智能导入题库 (Ctrl+I)");
    importAction->setStatusTip("AI自动识别格式、解析题目、生成测试数据");
    connect(importAction, &QAction::triggered, this, &MainWindow::onImportQuestionBank);
    
    // 注意：错误列表会在有错误时自动显示，无错误时自动隐藏
    // 上一题/下一题、运行测试、AI分析等按钮已在刷题模式界面中提供
    // 保持工具栏简洁
}

void MainWindow::setupMenuBar()
{
    // 文件菜单（简化版）
    QMenu *fileMenu = menuBar()->addMenu("文件(&F)");
    
    QAction *manageAction = fileMenu->addAction("📚 题库管理(&M)...");
    manageAction->setShortcut(QKeySequence("Ctrl+M"));
    manageAction->setStatusTip("管理所有题库：导入、切换、删除或查看题库信息");
    connect(manageAction, &QAction::triggered, this, &MainWindow::onManageQuestionBanks);
    
    fileMenu->addSeparator();
    
    QAction *exitAction = fileMenu->addAction("退出(&X)");
    exitAction->setShortcut(QKeySequence("Ctrl+Q"));
    connect(exitAction, &QAction::triggered, this, &QWidget::close);
    
    // 编辑菜单
    QMenu *editMenu = menuBar()->addMenu("编辑(&E)");
    
    QAction *undoAction = editMenu->addAction("撤销(&U)");
    undoAction->setShortcut(QKeySequence("Ctrl+Z"));
    undoAction->setStatusTip("撤销上一次操作");
    connect(undoAction, &QAction::triggered, this, &MainWindow::onUndo);
    
    QAction *redoAction = editMenu->addAction("重做(&R)");
    redoAction->setShortcut(QKeySequence("Ctrl+Shift+Z"));
    redoAction->setStatusTip("重做上一次撤销的操作");
    connect(redoAction, &QAction::triggered, this, &MainWindow::onRedo);
    
    editMenu->addSeparator();
    
    QAction *historyAction = editMenu->addAction("操作历史(&H)...");
    historyAction->setShortcut(QKeySequence("Ctrl+H"));
    historyAction->setStatusTip("查看操作历史记录");
    connect(historyAction, &QAction::triggered, this, &MainWindow::onShowOperationHistory);
    
    // 题目菜单
    QMenu *questionMenu = menuBar()->addMenu("题目(&Q)");
    
    QAction *viewOriginalAction = questionMenu->addAction("查看原题(&V)...");
    viewOriginalAction->setShortcut(QKeySequence("Ctrl+Shift+V"));
    viewOriginalAction->setStatusTip("查看题目的原始描述");
    connect(viewOriginalAction, &QAction::triggered, this, &MainWindow::onViewOriginalQuestion);
    
    questionMenu->addSeparator();
    
    QAction *generateAction = questionMenu->addAction("生成模拟题(&G)...");
    generateAction->setShortcut(QKeySequence("Ctrl+G"));
    generateAction->setStatusTip("使用AI基于现有题库生成一套模拟题");
    connect(generateAction, &QAction::triggered, this, &MainWindow::onGenerateExam);
    
    QAction *manageMockAction = questionMenu->addAction("模拟题库管理(&M)...");
    manageMockAction->setShortcut(QKeySequence("Ctrl+Shift+M"));
    manageMockAction->setStatusTip("管理生成的模拟题库");
    connect(manageMockAction, &QAction::triggered, this, &MainWindow::onManageMockExams);
    
    // 历史菜单
    QMenu *historyMenu = menuBar()->addMenu("历史(&H)");
    
    QAction *showHistoryAction = historyMenu->addAction("查看做题记录(&V)...");
    showHistoryAction->setShortcut(QKeySequence("Ctrl+H"));
    showHistoryAction->setStatusTip("查看历史做题记录和统计信息");
    connect(showHistoryAction, &QAction::triggered, this, &MainWindow::onShowHistory);
    
    // 代码菜单
    QMenu *codeMenu = menuBar()->addMenu("代码(&C)");
    
    QMenu *templateMenu = codeMenu->addMenu("插入模板(&T)");
    CodeTemplateManager &templateMgr = CodeTemplateManager::instance();
    for (const QString &templateName : templateMgr.templateNames()) {
        QAction *templateAction = templateMenu->addAction(templateName);
        connect(templateAction, &QAction::triggered, this, [this, templateName]() {
            onInsertTemplate(templateName);
        });
    }
    
    // 视图菜单
    QMenu *viewMenu = menuBar()->addMenu("视图(&V)");
    
    QAction *practiceModeAction = viewMenu->addAction("题库列表(&P)");
    practiceModeAction->setShortcut(QKeySequence("Ctrl+P"));
    connect(practiceModeAction, &QAction::triggered, this, &MainWindow::onSwitchToQuestionList);
    
    QAction *normalModeAction = viewMenu->addAction("刷题模式(&E)");
    normalModeAction->setShortcut(QKeySequence("Ctrl+E"));
    connect(normalModeAction, &QAction::triggered, this, &MainWindow::onSwitchToPracticeMode);
    
    viewMenu->addSeparator();
    
    QAction *aiAssistantAction = viewMenu->addAction("AI 助手面板(&A)");
    aiAssistantAction->setShortcut(QKeySequence("Ctrl+Shift+A"));
    aiAssistantAction->setStatusTip("显示/隐藏 AI 助手面板");
    aiAssistantAction->setCheckable(true);
    connect(aiAssistantAction, &QAction::triggered, this, &MainWindow::onToggleAIAssistant);
    
    QAction *codeVersionAction = viewMenu->addAction("代码版本历史(&H)...");
    codeVersionAction->setShortcut(QKeySequence("Ctrl+Shift+H"));
    codeVersionAction->setStatusTip("查看和恢复代码历史版本");
    connect(codeVersionAction, &QAction::triggered, this, &MainWindow::onShowCodeVersionHistory);
    
    // 工具菜单
    QMenu *toolsMenu = menuBar()->addMenu("工具(&T)");
    
    QAction *wrongBookAction = toolsMenu->addAction("错题本(&W)...");
    wrongBookAction->setShortcut(QKeySequence("Ctrl+W"));
    wrongBookAction->setStatusTip("查看和复习做错的题目");
    connect(wrongBookAction, &QAction::triggered, this, &MainWindow::onShowWrongBook);
    
    QAction *settingsAction = toolsMenu->addAction("设置(&S)...");
    settingsAction->setShortcut(QKeySequence("Ctrl+,"));
    settingsAction->setStatusTip("配置编译器、AI服务和编辑器选项");
    connect(settingsAction, &QAction::triggered, this, &MainWindow::onShowSettings);
    
    // 帮助菜单
    QMenu *helpMenu = menuBar()->addMenu("帮助(&H)");
    
    QAction *aboutAction = helpMenu->addAction("关于(&A)...");
    connect(aboutAction, &QAction::triggered, this, &MainWindow::onAbout);
}

void MainWindow::onSwitchToQuestionList()
{
    qDebug() << "Switching to question list...";
    qDebug() << "QuestionBank count:" << (m_questionBank ? m_questionBank->count() : -1);
    qDebug() << "PracticeWidget valid:" << (m_practiceWidget != nullptr);
    
    try {
        m_stackedWidget->setCurrentIndex(1);  // 切换到题库列表
        
        if (m_practiceWidget) {
            m_practiceWidget->refreshQuestionList();
        }
        
        statusBar()->showMessage("已切换到题库列表", 2000);
        qDebug() << "Successfully switched to question list";
    } catch (const std::exception &e) {
        qCritical() << "Exception in onSwitchToQuestionList:" << e.what();
        QMessageBox::critical(this, "错误", 
            QString("切换到题库列表时发生错误：\n%1").arg(e.what()));
    } catch (...) {
        qCritical() << "Unknown exception in onSwitchToQuestionList";
        QMessageBox::critical(this, "错误", "切换到题库列表时发生未知错误");
    }
}

void MainWindow::onSwitchToPracticeMode()
{
    m_stackedWidget->setCurrentIndex(0);  // 切换到刷题模式
    statusBar()->showMessage("已切换到刷题模式", 2000);
}

void MainWindow::onShowWrongBook()
{
    WrongQuestionWidget *wrongBookWidget = new WrongQuestionWidget(this);
    wrongBookWidget->setAttribute(Qt::WA_DeleteOnClose);
    wrongBookWidget->show();
}

void MainWindow::onFixTestCases()
{
    // 使用统一的测试用例修复工具
    TestCaseFixerDialog *dialog = new TestCaseFixerDialog(m_questionBank, m_ollamaClient, this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    
    // 连接信号，修复完成后自动刷新题库
    connect(dialog, &TestCaseFixerDialog::questionsFixed, this, [this]() {
        onRefreshQuestionBank();
    });
    
    dialog->exec();
}

void MainWindow::onBatchFixTestCases()
{
    // 重定向到统一的修复工具
    onFixTestCases();
}

void MainWindow::onShowCodeVersionHistory()
{
    if (m_currentQuestionIndex < 0 || m_currentQuestionIndex >= m_questionBank->count()) {
        QMessageBox::warning(this, "提示", "请先选择一道题目");
        return;
    }
    
    Question currentQuestion = m_questionBank->allQuestions()[m_currentQuestionIndex];
    QString questionId = currentQuestion.id();
    QString questionTitle = currentQuestion.title();
    
    CodeVersionDialog *dialog = new CodeVersionDialog(questionId, questionTitle, m_versionManager, this);
    
    // 连接恢复版本信号
    connect(dialog, &CodeVersionDialog::versionRestored, this, [this](const QString &code) {
        m_codeEditor->setCode(code);
        statusBar()->showMessage("代码版本已恢复", 3000);
    });
    
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->exec();
}

void MainWindow::onToggleAIAssistant()
{
    if (m_aiAssistantDock->isVisible()) {
        m_aiAssistantDock->hide();
    } else {
        m_aiAssistantDock->show();
        
        // 更新AI助手的题目上下文
        if (m_currentQuestionIndex >= 0 && m_currentQuestionIndex < m_questionBank->count()) {
            Question currentQuestion = m_questionBank->allQuestions()[m_currentQuestionIndex];
            m_aiAssistantPanel->setQuestionContext(currentQuestion);
        }
    }
}

void MainWindow::onViewOriginalQuestion()
{
    if (m_currentQuestionIndex < 0 || m_currentQuestionIndex >= m_questionBank->count()) {
        QMessageBox::warning(this, "提示", "请先选择一道题目");
        return;
    }
    
    Question currentQuestion = m_questionBank->allQuestions()[m_currentQuestionIndex];
    
    OriginalQuestionDialog *dialog = new OriginalQuestionDialog(currentQuestion, this);
    
    // 连接"开始练习"信号
    connect(dialog, &OriginalQuestionDialog::practiceRequested, this, [this]() {
        // 清空当前代码，重新开始
        if (m_currentQuestionIndex >= 0 && m_currentQuestionIndex < m_questionBank->count()) {
            Question currentQuestion = m_questionBank->allQuestions()[m_currentQuestionIndex];
            m_codeEditor->setCode(generateDefaultCode(currentQuestion));
        }
        statusBar()->showMessage("已清空代码，开始练习！", 3000);
    });
    
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
}

void MainWindow::onShowSettings()
{
    SettingsDialog dialog(this);
    
    // 连接AI配置更改信号
    connect(&dialog, &SettingsDialog::aiConfigChanged, this, [this]() {
        // 立即重新加载AI配置
        loadConfiguration();
        
        // 重新检测AI连接
        QTimer::singleShot(100, this, &MainWindow::checkAIConnection);
    });
    
    if (dialog.exec() == QDialog::Accepted) {
        // 重新加载配置
        loadConfiguration();
    }
}

void MainWindow::onInsertTemplate(const QString &templateName)
{
    CodeTemplateManager &templateMgr = CodeTemplateManager::instance();
    QString templateCode = templateMgr.getTemplate(templateName);
    m_codeEditor->setCode(templateCode);
    statusBar()->showMessage(QString("已插入模板: %1").arg(templateName), 3000);
}

void MainWindow::setupConnections()
{
    // 题库列表信号
    connect(m_practiceWidget, &PracticeWidget::questionSelected, this, [this](const Question &question) {
        qDebug() << "[MainWindow] Question selected from practice widget:" << question.id() << question.title();
        
        // 验证题目有效性
        if (question.id().isEmpty()) {
            qWarning() << "[MainWindow] Invalid question selected (empty id)";
            return;
        }
        
        qDebug() << "[MainWindow] Question details - Description length:" << question.description().length() 
                 << "Test cases:" << question.testCases().size();
        
        // 1. 保存当前题目的代码（如果有的话）
        if (m_codeEditor && !m_codeEditor->getQuestionId().isEmpty()) {
            qDebug() << "[MainWindow] Saving current code for question:" << m_codeEditor->getQuestionId();
            m_codeEditor->autoSaver()->forceSave();
        }
        
        // 2. 切换到刷题模式
        m_stackedWidget->setCurrentIndex(0);
        
        // 3. 设置题目到面板
        if (m_questionPanel) {
            qDebug() << "[MainWindow] Setting question to panel";
            m_questionPanel->setQuestion(question);
        } else {
            qWarning() << "[MainWindow] Question panel is null!";
        }
        
        // 4. 设置新题目ID到代码编辑器（这会触发AutoSaver加载保存的代码）
        if (m_codeEditor) {
            qDebug() << "[MainWindow] Setting question ID to editor:" << question.id();
            m_codeEditor->setQuestionId(question.id());
        }
        
        // 5. 加载保存的代码（如果AutoSaver没有加载，则手动加载）
        loadSavedCode(question.id());
        
        // 6. 尝试在 m_questionBank 中找到题目索引（用于导航）
        bool found = false;
        for (int i = 0; i < m_questionBank->count(); ++i) {
            if (m_questionBank->allQuestions()[i].id() == question.id()) {
                m_currentQuestionIndex = i;
                found = true;
                qDebug() << "[MainWindow] Found question in bank at index:" << i;
                break;
            }
        }
        
        if (!found) {
            qDebug() << "[MainWindow] Question not in current m_questionBank, but still loaded to panel";
            // 即使不在 m_questionBank 中，题目也已经加载到面板了
        }
        
        // 7. 保存会话状态（记住当前题目和面板状态）
        QString currentBankId = QuestionBankManager::instance().getCurrentBankId();
        if (!currentBankId.isEmpty()) {
            QuestionBankInfo bankInfo = QuestionBankManager::instance().getBankInfo(currentBankId);
            QString currentBankPath = bankInfo.path;
            if (!currentBankPath.isEmpty()) {
                // 使用题目ID作为会话标识，而不是索引
                SessionManager::instance().saveSession(currentBankPath, m_currentQuestionIndex, question.id());
                qDebug() << "[MainWindow] Session saved - Bank:" << currentBankPath << "Question:" << question.id();
            }
        }
        
        // 8. 保存题库面板状态
        if (m_questionBankPanel) {
            QStringList expandedPaths = m_questionBankPanel->getExpandedPaths();
            QString selectedPath = m_questionBankPanel->getSelectedQuestionPath();
            SessionManager::instance().savePanelState(expandedPaths, selectedPath);
        }
        
        statusBar()->showMessage(QString("已选择题目: %1").arg(question.title()), 3000);
    });
    
    // 题目面板信号
    connect(m_questionPanel, &QuestionPanel::runTests, 
            this, &MainWindow::onRunTests);
    connect(m_questionPanel, &QuestionPanel::nextQuestion, 
            this, &MainWindow::onNextQuestion);
    connect(m_questionPanel, &QuestionPanel::previousQuestion, 
            this, &MainWindow::onPreviousQuestion);
    connect(m_questionPanel, &QuestionPanel::aiJudgeRequested,
            this, &MainWindow::onAIJudgeRequested);
    
    // AI判题信号
    connect(m_aiJudge, &AIJudge::judgeCompleted,
            this, &MainWindow::onAIJudgeCompleted);
    connect(m_aiJudge, &AIJudge::error,
            this, &MainWindow::onAIJudgeError);
    
    // AI导师面板信号已在AIAssistantPanel内部处理
    
    // AI客户端信号
    connect(m_ollamaClient, &OllamaClient::codeAnalysisReady,
            this, &MainWindow::onAnalysisReady);
    connect(m_ollamaClient, &OllamaClient::error,
            this, &MainWindow::onAIError);
    
    // 代码编辑器信号
    connect(m_codeEditor, &CodeEditor::syntaxErrorsFound,
            this, &MainWindow::onSyntaxErrorsFound);
    m_codeEditor->setAIClient(m_ollamaClient);
    
    // 错误列表信号
    connect(m_errorListWidget, &ErrorListWidget::errorClicked,
            this, &MainWindow::onErrorClicked);
    
    // 题库信号
    connect(m_questionBank, &QuestionBank::questionsLoaded,
            this, &MainWindow::onQuestionsLoaded);
    
    // 题库树信号
    connect(m_questionBankPanel, &QuestionBankPanel::questionFileSelected,
            this, &MainWindow::onQuestionFileSelected);
    connect(m_questionBankPanel, &QuestionBankPanel::bankSelected,
            this, &MainWindow::onBankSelectedFromPanel);
    
    // 进度管理器信号 - 更新题目状态图标
    connect(&ProgressManager::instance(), &ProgressManager::progressUpdated,
            m_questionBankPanel, &QuestionBankPanel::updateQuestionStatus);
    
    // AI助手面板信号
    connect(m_aiAssistantPanel, &AIAssistantPanel::requestCurrentCode,
            this, [this]() {
        if (m_codeEditor) {
            QString code = m_codeEditor->code();
            m_aiAssistantPanel->setCurrentCode(code);
            qDebug() << "[MainWindow] Updated AI assistant with current code, length:" << code.length();
        }
    });
}

void MainWindow::loadConfiguration()
{
    ConfigManager &config = ConfigManager::instance();
    
    // 检测并配置编译器
    QString compilerPath = config.compilerPath();
    if (compilerPath.isEmpty() || !CompilerDetector::validateCompiler(compilerPath)) {
        // 自动检测编译器
        CompilerInfo bestCompiler = CompilerDetector::detectBestCompiler();
        if (bestCompiler.isValid) {
            compilerPath = bestCompiler.path;
            config.setCompilerPath(compilerPath);
            config.save();
            
            statusBar()->showMessage(
                QString("已自动检测到编译器: %1 %2")
                .arg(bestCompiler.name, bestCompiler.version), 5000);
        } else {
            QMessageBox::warning(this, "编译器未找到",
                "未检测到 C++ 编译器。\n\n"
                "请安装 MinGW 或 Clang，或在设置中手动指定编译器路径。\n\n"
                "程序将继续运行，但无法编译代码。");
        }
    }
    
    m_compilerRunner->setCompilerPath(compilerPath);
    
    // 配置AI服务
    if (config.useCloudApi()) {
        // 使用云端API
        m_ollamaClient->setCloudMode(true);
        m_ollamaClient->setBaseUrl(config.cloudApiUrl());
        m_ollamaClient->setModel(config.cloudApiModel());
        m_ollamaClient->setApiKey(config.cloudApiKey());
        qDebug() << "[MainWindow] 配置为云端API模式";
        qDebug() << "[MainWindow]   URL:" << config.cloudApiUrl();
        qDebug() << "[MainWindow]   Model:" << config.cloudApiModel();
        qDebug() << "[MainWindow]   API Key:" << (config.cloudApiKey().isEmpty() ? "未设置" : "已设置");
    } else {
        // 使用本地Ollama
        m_ollamaClient->setCloudMode(false);
        m_ollamaClient->setBaseUrl(config.ollamaUrl());
        m_ollamaClient->setModel(config.ollamaModel());
        qDebug() << "[MainWindow] 配置为本地Ollama模式";
        qDebug() << "[MainWindow]   URL:" << config.ollamaUrl();
        qDebug() << "[MainWindow]   Model:" << config.ollamaModel();
    }
}

void MainWindow::loadLastSession()
{
    QString questionBankPath;
    int questionIndex;
    QString questionId;
    
    if (SessionManager::instance().loadSession(questionBankPath, questionIndex, questionId)) {
        // 检查题库目录是否存在
        QDir bankDir(questionBankPath);
        if (!bankDir.exists()) {
            SessionManager::instance().clearSession();
            statusBar()->showMessage("上次的题库已被删除，请重新导入题库", 5000);
            return;
        }
        
        // 清空现有题库
        m_questionBank->clear();
        
        // 递归扫描所有子目录中的.json文件（支持分层结构）
        QStringList jsonFiles;
        
        // 先扫描根目录
        jsonFiles.append(bankDir.entryList(QStringList() << "*.json", QDir::Files));
        
        // 再扫描所有子目录
        QStringList subDirs = bankDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString &subDir : subDirs) {
            QDir subDirectory(questionBankPath + "/" + subDir);
            QStringList subFiles = subDirectory.entryList(QStringList() << "*.json", QDir::Files);
            for (const QString &file : subFiles) {
                jsonFiles.append(subDir + "/" + file);
            }
        }
        
        // 加载所有题目
        if (!jsonFiles.isEmpty()) {
            for (const QString &jsonFile : jsonFiles) {
                QString filePath = questionBankPath + "/" + jsonFile;
                QFile file(filePath);
                if (file.open(QIODevice::ReadOnly)) {
                    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
                    file.close();
                    
                    if (doc.isObject()) {
                        m_questionBank->addQuestion(Question(doc.object()));
                    }
                }
            }
            
            if (m_questionBank->count() > 0) {
                m_currentBankPath = questionBankPath;  // 记住当前题库路径
                m_questionBankPanel->refreshBankTree();
                
                // 恢复题库面板状态（展开的文件夹和选中的题目）
                QStringList expandedPaths;
                QString selectedQuestionPath;
                if (SessionManager::instance().loadPanelState(expandedPaths, selectedQuestionPath)) {
                    m_questionBankPanel->restoreExpandedPaths(expandedPaths);
                    if (!selectedQuestionPath.isEmpty()) {
                        m_questionBankPanel->selectQuestion(selectedQuestionPath);
                    }
                    qDebug() << "[MainWindow] Restored panel state - Expanded:" << expandedPaths.size() << "Selected:" << selectedQuestionPath;
                }
                
                // 优先使用题目ID查找题目
                bool foundById = false;
                if (!questionId.isEmpty()) {
                    for (int i = 0; i < m_questionBank->count(); ++i) {
                        if (m_questionBank->allQuestions()[i].id() == questionId) {
                            m_currentQuestionIndex = i;
                            foundById = true;
                            qDebug() << "[MainWindow] Found question by ID:" << questionId << "at index:" << i;
                            break;
                        }
                    }
                }
                
                // 如果没有找到题目ID，使用索引
                if (!foundById) {
                    m_currentQuestionIndex = qBound(0, questionIndex, m_questionBank->count() - 1);
                    qDebug() << "[MainWindow] Using question index:" << m_currentQuestionIndex;
                }
                
                loadCurrentQuestion();
                
                statusBar()->showMessage(
                    QString("✅ 已恢复上次会话：%1 道题目，当前第 %2 题")
                    .arg(m_questionBank->count())
                    .arg(m_currentQuestionIndex + 1), 5000);
            }
        } else {
            // 题库为空
            SessionManager::instance().clearSession();
            statusBar()->showMessage("题库为空，请重新导入题库", 5000);
        }
    }
}

void MainWindow::restoreWindowState()
{
    QByteArray geometry, state;
    if (SessionManager::instance().loadWindowState(geometry, state)) {
        restoreGeometry(geometry);
        restoreState(state);
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    // 保存会话（使用当前题库路径和题目ID）
    if (m_questionBank->count() > 0 && !m_currentBankPath.isEmpty()) {
        QString questionId;
        if (m_currentQuestionIndex >= 0 && m_currentQuestionIndex < m_questionBank->count()) {
            questionId = m_questionBank->allQuestions()[m_currentQuestionIndex].id();
        }
        SessionManager::instance().saveSession(m_currentBankPath, m_currentQuestionIndex, questionId);
        qDebug() << "[MainWindow] Saved session - Bank:" << m_currentBankPath << "Index:" << m_currentQuestionIndex << "ID:" << questionId;
    }
    
    // 保存窗口状态
    SessionManager::instance().saveWindowState(saveGeometry(), saveState());
    
    // 保存题库面板状态（展开的文件夹和选中的题目）
    if (m_questionBankPanel) {
        QStringList expandedPaths = m_questionBankPanel->getExpandedPaths();
        QString selectedPath = m_questionBankPanel->getSelectedQuestionPath();
        SessionManager::instance().savePanelState(expandedPaths, selectedPath);
        qDebug() << "[MainWindow] Saved panel state - Expanded:" << expandedPaths.size() << "Selected:" << selectedPath;
    }
    
    event->accept();
}

void MainWindow::onImportQuestionBank()
{
    // 只支持AI智能导入
    
    // 询问选择文件还是文件夹
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("选择导入方式");
    msgBox.setText("请选择要导入的题库类型：");
    msgBox.setIcon(QMessageBox::Question);
    
    QPushButton *folderBtn = msgBox.addButton("📁 选择文件夹", QMessageBox::ActionRole);
    QPushButton *filesBtn = msgBox.addButton("📄 选择文件", QMessageBox::ActionRole);
    QPushButton *cancelBtn = msgBox.addButton("取消", QMessageBox::RejectRole);
    
    msgBox.exec();
    
    QString path;
    
    if (msgBox.clickedButton() == folderBtn) {
        // 选择文件夹
        path = QFileDialog::getExistingDirectory(
            this,
            "选择题库文件夹",
            QDir::homePath(),
            QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
        );
    } else if (msgBox.clickedButton() == filesBtn) {
        // 选择文件
        QStringList files = QFileDialog::getOpenFileNames(
            this,
            "选择题库文件",
            QDir::homePath(),
            "题库文件 (*.md *.markdown *.txt);;所有文件 (*.*)"
        );
        
        if (!files.isEmpty()) {
            // 创建临时文件夹，将选中的文件复制进去
            QString tempDir = QDir::tempPath() + "/CodePracticeSystem_Import_" + 
                             QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
            QDir().mkpath(tempDir);
            
            for (const QString &file : files) {
                QFile::copy(file, tempDir + "/" + QFileInfo(file).fileName());
            }
            
            path = tempDir;
        }
    } else {
        return;
    }
    
    if (path.isEmpty()) {
        return;
    }
    
    // 询问题库名称（分类名称）
    bool ok;
    QString categoryName = QInputDialog::getText(
        this, 
        "题库名称",
        "请输入题库名称（如：CCF考试、LeetCode）:\n\n"
        "💡 提示：\n"
        "• 输入新名称：创建新题库\n"
        "• 输入已有名称：导入到现有题库（同名题目会被覆盖）",
        QLineEdit::Normal,
        QFileInfo(path).fileName(), 
        &ok
    );
    
    if (!ok || categoryName.isEmpty()) {
        return;
    }
    
    // 检查题库是否已存在
    QString bankPath = QString("data/基础题库/%1").arg(categoryName);
    bool bankExists = QDir(bankPath).exists();
    
    if (bankExists) {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this,
            "题库已存在",
            QString("题库【%1】已存在！\n\n"
                    "导入操作将：\n"
                    "• 保留现有题目\n"
                    "• 添加新题目\n"
                    "• 覆盖同名题目\n\n"
                    "是否继续？").arg(categoryName),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::Yes
        );
        
        if (reply != QMessageBox::Yes) {
            return;
        }
    }
    
    // 使用AI智能导入
    SmartImportDialog *smartDialog = new SmartImportDialog(path, categoryName, m_ollamaClient, this);
    if (smartDialog->exec() == QDialog::Accepted && smartDialog->isSuccess()) {
        // SmartQuestionImporter已经保存了所有数据：
        // 1. data/原始题库/{categoryName}/ - 只读备份
        // 2. data/基础题库/{categoryName}/questions.json - AI解析后的JSON题库（主要使用）
        // 3. data/基础题库/{categoryName}/*.md - Markdown格式（查看备份）
        // 4. data/config/ccf_parse_rule.json - 解析规则
        
        // 从基础题库加载JSON（支持分层结构）
        QString bankPath = QString("data/基础题库/%1").arg(categoryName);
        
        // 清空现有题库
        m_questionBank->clear();
        
        // 递归扫描所有子目录中的.json文件
        QDir bankDir(bankPath);
        QStringList jsonFiles;
        
        // 先扫描根目录
        jsonFiles.append(bankDir.entryList(QStringList() << "*.json", QDir::Files));
        
        // 再扫描所有子目录
        QStringList subDirs = bankDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString &subDir : subDirs) {
            QDir subDirectory(bankPath + "/" + subDir);
            QStringList subFiles = subDirectory.entryList(QStringList() << "*.json", QDir::Files);
            for (const QString &file : subFiles) {
                jsonFiles.append(subDir + "/" + file);
            }
        }
        
        if (!jsonFiles.isEmpty()) {
            // 加载所有题目
            for (const QString &jsonFile : jsonFiles) {
                QString filePath = bankPath + "/" + jsonFile;
                QFile file(filePath);
                if (file.open(QIODevice::ReadOnly)) {
                    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
                    file.close();
                    
                    if (doc.isObject()) {
                        m_questionBank->addQuestion(Question(doc.object()));
                    }
                }
            }
        }
        
        // 注册题库到QuestionBankManager（如果是新题库）
        if (!bankExists) {
            QString bankId = QuestionBankManager::instance().importQuestionBank(bankPath, categoryName, true);
            qDebug() << "题库已注册到管理器，ID:" << bankId;
        } else {
            // 如果题库已存在，更新题目数量
            QVector<QuestionBankInfo> banks = QuestionBankManager::instance().getAllBanks();
            for (const QuestionBankInfo &info : banks) {
                if (info.name == categoryName) {
                    // 重新统计题目数量
                    int questionCount = 0;
                    QDir dir(bankPath);
                    QStringList filters;
                    filters << "*.json";
                    
                    // 递归统计所有JSON文件中的题目
                    std::function<void(const QString&)> countQuestions = [&](const QString &path) {
                        QDir currentDir(path);
                        QFileInfoList files = currentDir.entryInfoList(filters, QDir::Files);
                        for (const auto &fileInfo : files) {
                            QFile file(fileInfo.absoluteFilePath());
                            if (file.open(QIODevice::ReadOnly)) {
                                QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
                                if (doc.isArray()) {
                                    questionCount += doc.array().size();
                                } else if (doc.isObject()) {
                                    questionCount += 1;
                                }
                                file.close();
                            }
                        }
                        
                        QFileInfoList subDirs = currentDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
                        for (const auto &subDirInfo : subDirs) {
                            countQuestions(subDirInfo.absoluteFilePath());
                        }
                    };
                    
                    countQuestions(bankPath);
                    QuestionBankManager::instance().updateQuestionCount(info.id, questionCount);
                    qDebug() << "题库题目数量已更新:" << categoryName << "共" << questionCount << "道题目";
                    break;
                }
            }
        }
        
        // 更新UI
        m_questionBankPanel->refreshBankTree();
        
        if (m_questionBank->count() > 0) {
            m_currentQuestionIndex = 0;
            m_currentBankPath = bankPath;  // 记住当前题库路径
            loadCurrentQuestion();
            
            // 保存会话状态（记住当前题库路径）
            SessionManager::instance().saveSession(bankPath, 0);
            
            // 统计测试数据
            int totalTestCases = 0;
            int aiGeneratedCases = 0;
            for (const Question &q : m_questionBank->allQuestions()) {
                totalTestCases += q.testCases().size();
                for (const TestCase &tc : q.testCases()) {
                    if (tc.isAIGenerated) {
                        aiGeneratedCases++;
                    }
                }
            }
            
            statusBar()->showMessage(
                QString("✅ 【%1】题库导入成功！共 %2 道题目，%3 组测试数据（AI生成 %4 组）")
                .arg(categoryName)
                .arg(m_questionBank->count())
                .arg(totalTestCases)
                .arg(aiGeneratedCases), 8000);
            
            QMessageBox::information(this, "导入成功",
                QString("【%1】题库导入成功！\n\n"
                        "📊 题库统计：\n"
                        "• 总题数：%2 道\n"
                        "• 测试数据：%3 组（原始 %4 组 + AI生成 %5 组）\n\n"
                        "📁 已生成文件：\n"
                        "• 原始题库（只读）：data/原始题库/%1/\n"
                        "• 基础题库（JSON）：%6\n"
                        "• 解析规则：data/config/ccf_parse_rule.json\n\n"
                        "✅ 现在可以直接刷题或生成模拟题！")
                .arg(categoryName)
                .arg(m_questionBank->count())
                .arg(totalTestCases)
                .arg(totalTestCases - aiGeneratedCases)
                .arg(aiGeneratedCases)
                .arg(bankPath));
        } else {
            QMessageBox::warning(this, "加载失败", 
                QString("未找到题库文件，请检查目录：%1").arg(bankPath));
        }
    }
    smartDialog->deleteLater();
}

void MainWindow::onRefreshQuestionBank()
{
    // 刷新当前题库（重新加载JSON）
    if (m_questionBank->count() == 0) {
        QMessageBox::information(this, "提示", "当前没有题库，请先导入题库");
        return;
    }
    
    int currentIndex = m_currentQuestionIndex;
    
    // 重新加载题库
    m_questionBank->loadFromDirectory("data/questions");
    
    if (m_questionBank->count() > 0) {
        // 恢复当前题目位置
        m_currentQuestionIndex = qBound(0, currentIndex, m_questionBank->count() - 1);
        loadCurrentQuestion();
        
        statusBar()->showMessage(
            QString("题库已刷新：共 %1 道题目").arg(m_questionBank->count()), 3000);
    } else {
        QMessageBox::warning(this, "刷新失败", "题库文件不存在或为空");
    }
}

void MainWindow::onRefreshCurrentBank()
{
    // 刷新当前题库（从当前路径重新加载）
    if (m_currentBankPath.isEmpty()) {
        QMessageBox::information(this, "提示", "当前没有题库，请先导入题库");
        return;
    }
    
    int currentIndex = m_currentQuestionIndex;
    
    // 清空并重新加载
    m_questionBank->clear();
    m_questionBank->loadFromDirectory(m_currentBankPath);
    
    if (m_questionBank->count() > 0) {
        // 恢复当前题目位置
        m_currentQuestionIndex = qBound(0, currentIndex, m_questionBank->count() - 1);
        m_questionBankPanel->refreshBankTree();
        loadCurrentQuestion();
        
        // 刷新刷题模式
        if (m_practiceWidget) {
            m_practiceWidget->refreshQuestionList();
        }
        
        statusBar()->showMessage(
            QString("题库已刷新：共 %1 道题目").arg(m_questionBank->count()), 3000);
    } else {
        QMessageBox::warning(this, "刷新失败", "题库文件不存在或为空");
    }
}

void MainWindow::onReloadQuestionBank()
{
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "重新加载题库",
        "重新加载会清空当前题库并重新导入。\n\n"
        "是否继续？",
        QMessageBox::Yes | QMessageBox::No
    );
    
    if (reply == QMessageBox::Yes) {
        // 询问题库路径
        QString path = QFileDialog::getExistingDirectory(
            this,
            "选择题库文件夹",
            m_lastImportPath.isEmpty() ? "" : m_lastImportPath
        );
        
        if (!path.isEmpty()) {
            // 清空当前题库
            m_questionBank->clear();
            m_currentQuestionIndex = -1;
            
            // 重新导入
            importQuestionsFromPath(path);
        }
    }
}

void MainWindow::onManageQuestionBanks()
{
    QuestionBankManagerDialog *dialog = new QuestionBankManagerDialog(m_ollamaClient, this);
    
    // 连接信号
    connect(dialog, &QuestionBankManagerDialog::bankDeleted, this, [this](const QString &bankId) {
        // 如果删除的是当前题库，清空
        if (QuestionBankManager::instance().getCurrentBankId() == bankId) {
            m_questionBank->clear();
            m_currentQuestionIndex = -1;
            m_questionPanel->setQuestion(Question());
            m_codeEditor->setCode("");
            m_questionBankPanel->refreshBankTree();
            m_practiceWidget->refreshQuestionList();
        }
    });
    
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->exec();
}

void MainWindow::onClearQuestionBank()
{
    if (m_questionBank->count() == 0) {
        QMessageBox::information(this, "提示", "当前没有题库");
        return;
    }
    
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("清空题库");
    msgBox.setText("确定要清空当前题库吗？");
    msgBox.setInformativeText(QString("当前有 %1 道题目，清空后将无法恢复。\n\n"
                                     "注意：这不会删除原始文件，只是清空程序中的题库。")
                             .arg(m_questionBank->count()));
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);
    
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
        QPushButton:pressed {
            background-color: #440000;
        }
    )");
    
    if (msgBox.exec() == QMessageBox::Yes) {
        // 清空题库
        m_questionBank->clear();
        m_currentQuestionIndex = -1;
        
        // 清空UI
        m_questionPanel->setQuestion(Question());
        m_codeEditor->setCode("");
        m_questionBankPanel->refreshBankTree();
        
        // 刷新刷题模式
        m_practiceWidget->refreshQuestionList();
        
        // 清空会话
        SessionManager::instance().clearSession();
        
        statusBar()->showMessage("题库已清空", 3000);
        
        QMessageBox::information(this, "完成", 
            "题库已清空。\n\n"
            "您可以重新导入题库。");
    }
}

void MainWindow::importQuestionsFromPath(const QString &path)
{
    // 保存路径
    m_lastImportPath = path;
    
    // 解析Markdown文件
    QuestionParser parser;
    QDir dir(path);
    QStringList filters;
    filters << "*.md" << "*.markdown";
    QFileInfoList files = dir.entryInfoList(filters, QDir::Files);
    
    if (files.isEmpty()) {
        QMessageBox::warning(this, "警告", "所选文件夹中没有找到 Markdown 文件");
        return;
    }
    
    int totalQuestions = 0;
    int failedFiles = 0;
    
    for (const auto &fileInfo : files) {
        try {
            QVector<Question> questions = parser.parseMarkdownFile(fileInfo.absoluteFilePath());
            if (questions.isEmpty()) {
                failedFiles++;
                continue;
            }
            
            for (const auto &q : questions) {
                m_questionBank->addQuestion(q);
                totalQuestions++;
            }
        } catch (...) {
            failedFiles++;
        }
    }
    
    // 保存题库到JSON
    saveQuestionBank();
    
    // 显示导入结果
    QString message = QString("成功导入 %1 道题目").arg(totalQuestions);
    if (failedFiles > 0) {
        message += QString("\n\n%1 个文件解析失败").arg(failedFiles);
    }
    
    QMessageBox::information(this, "导入完成", message);
    
    // 加载第一题
    if (totalQuestions > 0) {
        m_currentQuestionIndex = 0;
        loadCurrentQuestion();
    }
}

void MainWindow::onShowHistory()
{
    HistoryWidget *historyWidget = new HistoryWidget();
    historyWidget->setAttribute(Qt::WA_DeleteOnClose);
    historyWidget->show();
}

void MainWindow::onGenerateExam()
{
    if (m_questionBank->count() == 0) {
        QMessageBox::warning(this, "提示", 
            "当前没有题库，无法生成模拟题。\n\n"
            "请先导入题库作为参考。");
        return;
    }
    
    // 创建生成对话框
    ExamGeneratorDialog *dialog = new ExamGeneratorDialog(
        m_questionBank->allQuestions(), 
        m_ollamaClient, 
        this
    );
    
    if (dialog->exec() == QDialog::Accepted && dialog->isSuccess()) {
        QVector<Question> generatedQuestions = dialog->getGeneratedQuestions();
        
        if (!generatedQuestions.isEmpty()) {
            // 询问是否添加到当前题库
            QMessageBox::StandardButton reply = QMessageBox::question(
                this,
                "添加到题库",
                QString("成功生成 %1 道模拟题！\n\n"
                       "是否将这些题目添加到当前题库？").arg(generatedQuestions.size()),
                QMessageBox::Yes | QMessageBox::No
            );
            
            if (reply == QMessageBox::Yes) {
                // 添加到题库
                for (const Question &q : generatedQuestions) {
                    m_questionBank->addQuestion(q);
                }
                
                // 更新UI
                m_questionBankPanel->refreshBankTree();
                
                // 刷新刷题模式
                m_practiceWidget->refreshQuestionList();
                
                statusBar()->showMessage(
                    QString("✅ 已添加 %1 道模拟题到题库").arg(generatedQuestions.size()), 
                    5000
                );
                
                QMessageBox::information(this, "添加成功",
                    QString("已成功添加 %1 道模拟题到题库！\n\n"
                           "现在可以开始练习了。").arg(generatedQuestions.size()));
            }
        }
    }
    
    dialog->deleteLater();
}

void MainWindow::onManageMockExams()
{
    if (m_questionBank->count() == 0) {
        QMessageBox::warning(this, "提示", 
            "当前没有题库，无法管理模拟题。\n\n"
            "请先导入题库。");
        return;
    }
    
    // 创建模拟题管理对话框
    // 传入当前题库的所有题目和AI客户端
    MockExamManagerDialog *dialog = new MockExamManagerDialog(
        m_questionBank->allQuestions(),
        m_ollamaClient,
        this
    );
    
    dialog->exec();
    dialog->deleteLater();
}

void MainWindow::onAbout()
{
    QMessageBox::about(this, "关于",
        "<h3>代码刷题系统 v2.0</h3>"
        "<p>基于Qt6的智能刷题系统</p>"
        "<p>支持本地AI模型（Ollama）和云端API</p>"
        "<p><b>新功能：</b></p>"
        "<ul>"
        "<li>智能题库导入</li>"
        "<li>AI生成模拟题</li>"
        "<li>LeetCode风格测试</li>"
        "<li>完整测试数据</li>"
        "</ul>"
        "<p><b>技术栈：</b></p>"
        "<ul>"
        "<li>Qt 6.10.0</li>"
        "<li>QScintilla 2.14.1</li>"
        "<li>C++17</li>"
        "<li>Ollama AI</li>"
        "</ul>");
}

void MainWindow::onUndo()
{
    if (OperationHistory::instance().canUndo()) {
        if (OperationHistory::instance().undo()) {
            // 刷新题库面板
            if (m_questionBankPanel) {
                m_questionBankPanel->refreshBankTree();
            }
            statusBar()->showMessage("✅ 操作已撤销", 3000);
        } else {
            QMessageBox::warning(this, "撤销失败", "无法撤销此操作");
        }
    } else {
        statusBar()->showMessage("没有可撤销的操作", 2000);
    }
}

void MainWindow::onRedo()
{
    if (OperationHistory::instance().canRedo()) {
        if (OperationHistory::instance().redo()) {
            // 刷新题库面板
            if (m_questionBankPanel) {
                m_questionBankPanel->refreshBankTree();
            }
            statusBar()->showMessage("✅ 操作已重做", 3000);
        } else {
            QMessageBox::warning(this, "重做失败", "无法重做此操作");
        }
    } else {
        statusBar()->showMessage("没有可重做的操作", 2000);
    }
}

void MainWindow::onShowOperationHistory()
{
    QVector<Operation> history = OperationHistory::instance().getHistory();
    int currentIndex = OperationHistory::instance().getCurrentIndex();
    
    if (history.isEmpty()) {
        QMessageBox::information(this, "操作历史", "暂无操作历史记录");
        return;
    }
    
    // 创建对话框显示历史
    QDialog *dialog = new QDialog(this);
    dialog->setWindowTitle("操作历史");
    dialog->resize(600, 400);
    
    QVBoxLayout *layout = new QVBoxLayout(dialog);
    
    QListWidget *listWidget = new QListWidget(dialog);
    listWidget->setStyleSheet(R"(
        QListWidget {
            background-color: #2d2d2d;
            color: #e8e8e8;
            border: 1px solid #3a3a3a;
            border-radius: 4px;
        }
        QListWidget::item {
            padding: 8px;
            border-bottom: 1px solid #3a3a3a;
        }
        QListWidget::item:selected {
            background-color: #660000;
        }
    )");
    
    for (int i = 0; i < history.size(); ++i) {
        const Operation &op = history[i];
        QString prefix = (i == currentIndex) ? "→ " : "  ";
        QString text = QString("%1%2 - %3")
            .arg(prefix)
            .arg(op.timestamp.toString("yyyy-MM-dd hh:mm:ss"))
            .arg(op.description);
        
        QListWidgetItem *item = new QListWidgetItem(text, listWidget);
        if (i == currentIndex) {
            item->setForeground(QColor("#ffff00"));  // 黄色高亮当前位置
        } else if (i > currentIndex) {
            item->setForeground(QColor("#888888"));  // 灰色表示已撤销
        }
    }
    
    layout->addWidget(listWidget);
    
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    
    QPushButton *clearButton = new QPushButton("清空历史", dialog);
    clearButton->setStyleSheet(R"(
        QPushButton {
            background-color: #8b0000;
            color: white;
            border: none;
            border-radius: 4px;
            padding: 8px 16px;
        }
        QPushButton:hover {
            background-color: #a00000;
        }
    )");
    connect(clearButton, &QPushButton::clicked, [this, dialog]() {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this,
            "确认清空",
            "确定要清空所有操作历史吗？\n\n此操作不可撤销！",
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No
        );
        
        if (reply == QMessageBox::Yes) {
            OperationHistory::instance().clear();
            dialog->accept();
            QMessageBox::information(this, "成功", "操作历史已清空");
        }
    });
    
    QPushButton *closeButton = new QPushButton("关闭", dialog);
    closeButton->setStyleSheet(R"(
        QPushButton {
            background-color: #3a3a3a;
            color: white;
            border: none;
            border-radius: 4px;
            padding: 8px 16px;
        }
        QPushButton:hover {
            background-color: #4a4a4a;
        }
    )");
    connect(closeButton, &QPushButton::clicked, dialog, &QDialog::accept);
    
    buttonLayout->addWidget(clearButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(closeButton);
    
    layout->addLayout(buttonLayout);
    
    dialog->setStyleSheet("QDialog { background-color: #242424; }");
    dialog->exec();
    delete dialog;
}

void MainWindow::onRunTests()
{
    if (m_currentQuestionIndex < 0 || m_currentQuestionIndex >= m_questionBank->count()) {
        QMessageBox::warning(this, "警告", "没有加载题目");
        return;
    }
    
    QString code = m_codeEditor->code();
    if (code.trimmed().isEmpty()) {
        QMessageBox::warning(this, "警告", "请先编写代码");
        return;
    }
    
    // 编译代码
    CompileResult compileResult = m_compilerRunner->compile(code);
    
    if (!compileResult.success) {
        ErrorHandler::handleCompileError(this, compileResult.error);
        return;
    }
    
    // 运行测试
    Question currentQuestion = m_questionBank->allQuestions()[m_currentQuestionIndex];
    QVector<TestCase> testCases = currentQuestion.testCases();
    
    if (testCases.isEmpty()) {
        QMessageBox::information(this, "提示", "该题目没有测试用例");
        return;
    }
    
    // 获取可执行文件路径（从编译结果推断）
    QString exePath = QDir::tempPath() + "/code.exe";
    QVector<TestResult> results = m_compilerRunner->runTests(exePath, testCases);
    
    // 显示测试结果
    showTestResults(results);
}

void MainWindow::onAIJudgeRequested()
{
    if (m_currentQuestionIndex < 0 || m_currentQuestionIndex >= m_questionBank->count()) {
        QMessageBox::warning(this, "警告", "没有加载题目");
        return;
    }
    
    Question currentQuestion = m_questionBank->allQuestions()[m_currentQuestionIndex];
    QString questionId = currentQuestion.id();
    
    // 获取编辑器中的代码
    // 注意：编辑器中的代码已经从 data/user_answers/{questionId}.cpp 加载
    // 并且会自动保存到该文件，所以这里获取的就是用户保存的代码
    QString code = m_codeEditor->code();
    
    if (code.trimmed().isEmpty()) {
        QMessageBox::warning(this, "警告", "请先编写代码");
        return;
    }
    
    qDebug() << "[MainWindow] AI judge requested for question:" << questionId 
             << "Code length:" << code.length();
    
    // 强制保存当前代码（确保最新代码已保存）
    m_codeEditor->forceSave();
    
    // 显示进度对话框
    if (!m_aiJudgeProgressDialog) {
        m_aiJudgeProgressDialog = new QProgressDialog(this);
        m_aiJudgeProgressDialog->setWindowTitle("AI判题中");
        m_aiJudgeProgressDialog->setLabelText("正在分析代码...");
        m_aiJudgeProgressDialog->setRange(0, 0);  // 不确定进度
        m_aiJudgeProgressDialog->setModal(true);
        m_aiJudgeProgressDialog->setCancelButton(nullptr);  // 不允许取消
        m_aiJudgeProgressDialog->setMinimumWidth(300);
        m_aiJudgeProgressDialog->setMinimumHeight(120);
    }
    
    // 手动居中对话框
    QRect parentRect = this->geometry();
    QSize dialogSize = m_aiJudgeProgressDialog->sizeHint();
    int x = parentRect.x() + (parentRect.width() - dialogSize.width()) / 2;
    int y = parentRect.y() + (parentRect.height() - dialogSize.height()) / 2;
    m_aiJudgeProgressDialog->move(x, y);
    
    m_aiJudgeProgressDialog->show();
    
    // 开始AI判题
    m_aiJudge->judgeCode(currentQuestion, code);
}

void MainWindow::onAIJudgeCompleted(bool passed, const QString &comment, const QVector<int> &failedTestCases)
{
    // 关闭进度对话框
    if (m_aiJudgeProgressDialog) {
        m_aiJudgeProgressDialog->hide();
    }
    
    // 获取当前题目
    if (m_currentQuestionIndex < 0 || m_currentQuestionIndex >= m_questionBank->count()) {
        qWarning() << "[MainWindow] Invalid question index in onAIJudgeCompleted";
        return;
    }
    
    Question currentQuestion = m_questionBank->allQuestions()[m_currentQuestionIndex];
    QString questionId = currentQuestion.id();
    
    qDebug() << "[MainWindow] AI judge completed for question:" << questionId 
             << "Passed:" << passed;
    
    // 更新进度管理器
    ProgressManager &progressMgr = ProgressManager::instance();
    
    // 记录AI判定结果
    progressMgr.recordAIJudge(questionId, passed, comment);
    
    // 保存当前代码
    QString code = m_codeEditor->code();
    progressMgr.saveLastCode(questionId, code);
    
    // 更新题目状态
    if (passed) {
        // AI判定通过，更新为已完成
        progressMgr.updateStatus(questionId, QuestionStatus::Completed);
        qDebug() << "[MainWindow] Updated question status to Completed";
    } else {
        // AI判定未通过，更新为进行中
        progressMgr.updateStatus(questionId, QuestionStatus::InProgress);
        qDebug() << "[MainWindow] Updated question status to InProgress";
    }
    
    // 保存进度
    progressMgr.save();
    
    // 通知题库面板更新状态（通过信号）
    // ProgressManager 会发出 progressUpdated 信号，题库面板已连接
    
    // 显示结果
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("AI判题结果");
    
    if (passed) {
        msgBox.setIcon(QMessageBox::Information);
        msgBox.setText("🎉 AI判定通过！");
        msgBox.setInformativeText(QString("评论：\n%1\n\n✅ 已自动更新题目状态为\"已完成\"").arg(comment));
        msgBox.setStandardButtons(QMessageBox::Ok);
    } else {
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setText("❌ AI判定未通过");
        
        QString failedInfo;
        if (!failedTestCases.isEmpty()) {
            failedInfo = QString("\n\n未通过的测试用例：%1").arg(
                [&failedTestCases]() {
                    QStringList list;
                    for (int idx : failedTestCases) {
                        list << QString::number(idx);
                    }
                    return list.join(", ");
                }()
            );
        }
        
        msgBox.setInformativeText(QString("AI分析：\n%1%2\n\n⚠️ 题目状态已更新为\"进行中\"，请根据建议修改代码后重试。")
            .arg(comment, failedInfo));
        msgBox.setStandardButtons(QMessageBox::Ok);
    }
    
    msgBox.exec();
    
    // 刷新题库面板显示（确保状态图标更新）
    if (m_questionBankPanel) {
        m_questionBankPanel->updateQuestionStatus(questionId);
    }
    
    // 刷新题库列表（如果在题库列表视图）
    if (m_practiceWidget && m_stackedWidget->currentIndex() == 1) {
        m_practiceWidget->refreshQuestionList();
    }
}

void MainWindow::onAIJudgeError(const QString &error)
{
    // 关闭进度对话框
    if (m_aiJudgeProgressDialog) {
        m_aiJudgeProgressDialog->hide();
    }
    
    QMessageBox::critical(this, "AI判题错误", 
        QString("AI判题过程中发生错误：\n%1").arg(error));
}

void MainWindow::onNextQuestion()
{
    if (m_questionBank->count() == 0) {
        QMessageBox::warning(this, "警告", "没有题目");
        return;
    }
    
    m_currentQuestionIndex++;
    if (m_currentQuestionIndex >= m_questionBank->count()) {
        m_currentQuestionIndex = 0; // 循环到第一题
    }
    
    loadCurrentQuestion();
}

void MainWindow::onPreviousQuestion()
{
    if (m_questionBank->count() == 0) {
        QMessageBox::warning(this, "警告", "没有题目");
        return;
    }
    
    m_currentQuestionIndex--;
    if (m_currentQuestionIndex < 0) {
        m_currentQuestionIndex = m_questionBank->count() - 1; // 循环到最后一题
    }
    
    loadCurrentQuestion();
}

void MainWindow::onRequestAnalysis()
{
    if (m_currentQuestionIndex < 0) {
        QMessageBox::warning(this, "警告", "没有加载题目");
        return;
    }
    
    QString code = m_codeEditor->code();
    if (code.trimmed().isEmpty()) {
        QMessageBox::warning(this, "警告", "请先编写代码");
        return;
    }
    
    Question currentQuestion = m_questionBank->allQuestions()[m_currentQuestionIndex];
    // 旧的分析方式已废弃，现在使用对话模式
    // 用户可以在AI导师面板中点击"分析代码"按钮
    QMessageBox::information(this, "提示", 
        "AI分析功能已升级为对话模式！\n\n"
        "请在右侧AI导师面板中：\n"
        "1. 点击「💡 分析代码」按钮\n"
        "2. 或直接输入你的问题\n\n"
        "AI导师会通过对话引导你思考和解决问题。");
}

void MainWindow::onAnalysisReady(const QString &analysis)
{
    // 旧的分析方式已废弃，现在使用对话模式
    // AI响应会通过流式输出显示在对话面板中
}

void MainWindow::onAIError(const QString &error)
{
    // 错误会通过AI面板的错误处理机制显示
    ErrorHandler::handleNetworkError(this, error);
}

void MainWindow::onQuestionsLoaded(int count)
{
    statusBar()->showMessage(QString("已加载 %1 道题目").arg(count), 3000);
    
    // 更新题目列表
    m_questionBankPanel->refreshBankTree();
}

void MainWindow::onQuestionSelectedFromList(int index)
{
    if (index >= 0 && index < m_questionBank->count()) {
        m_currentQuestionIndex = index;
        loadCurrentQuestion();
    }
}

void MainWindow::onQuestionFileSelected(const QString &filePath, const Question &question)
{
    qDebug() << "[MainWindow] Question file selected:" << filePath << question.id();
    
    // 1. 保存当前题目的代码（如果有的话）
    if (m_codeEditor && !m_codeEditor->getQuestionId().isEmpty()) {
        qDebug() << "[MainWindow] Saving current code for question:" << m_codeEditor->getQuestionId();
        m_codeEditor->autoSaver()->forceSave();
    }
    
    // 2. 加载题目到面板
    m_questionPanel->setQuestion(question);
    
    // 3. 设置新题目ID到代码编辑器
    if (m_codeEditor) {
        qDebug() << "[MainWindow] Setting question ID to editor:" << question.id();
        m_codeEditor->setQuestionId(question.id());
    }
    
    // 4. 加载保存的代码或使用模板
    QString savedCode = loadSavedCodeForQuestion(question.id());
    if (savedCode.isEmpty()) {
        savedCode = generateDefaultCode(question);
    }
    m_codeEditor->setCode(savedCode);
    
    // 5. 尝试在 m_questionBank 中找到题目索引（用于导航）
    bool found = false;
    for (int i = 0; i < m_questionBank->count(); ++i) {
        if (m_questionBank->allQuestions()[i].id() == question.id()) {
            m_currentQuestionIndex = i;
            found = true;
            qDebug() << "[MainWindow] Found question in bank at index:" << i;
            break;
        }
    }
    
    if (!found) {
        qDebug() << "[MainWindow] Question not in current m_questionBank";
    }
    
    // 6. 保存会话状态（记住当前题目和面板状态）
    QString currentBankId = QuestionBankManager::instance().getCurrentBankId();
    if (!currentBankId.isEmpty()) {
        QuestionBankInfo bankInfo = QuestionBankManager::instance().getBankInfo(currentBankId);
        QString currentBankPath = bankInfo.path;
        if (!currentBankPath.isEmpty()) {
            // 保存会话
            SessionManager::instance().saveSession(currentBankPath, m_currentQuestionIndex, question.id());
            qDebug() << "[MainWindow] Session saved - Bank:" << currentBankPath << "Question:" << question.id();
        }
    }
    
    // 7. 保存题库面板状态
    if (m_questionBankPanel) {
        QStringList expandedPaths = m_questionBankPanel->getExpandedPaths();
        QString selectedPath = m_questionBankPanel->getSelectedQuestionPath();
        SessionManager::instance().savePanelState(expandedPaths, selectedPath);
        qDebug() << "[MainWindow] Panel state saved - Expanded:" << expandedPaths.size() << "Selected:" << selectedPath;
    }
    
    // 8. 更新AI助手的题目上下文
    if (m_aiAssistantPanel) {
        qDebug() << "[MainWindow] Updating AI assistant context for question:" << question.id();
        m_aiAssistantPanel->setQuestionContext(question);
    } else {
        qWarning() << "[MainWindow] m_aiAssistantPanel is null!";
    }
    
    // 更新状态栏
    statusBar()->showMessage(QString("✅ 已加载题目：%1").arg(question.title()), 3000);
}

void MainWindow::onBankSelectedFromPanel(const QString &bankPath)
{
    qDebug() << "[MainWindow] Bank selected from panel:" << bankPath;
    // 可以在这里添加题库选中的处理逻辑
}

QString MainWindow::loadSavedCodeForQuestion(const QString &questionId)
{
    // 从 .cpp 文件加载保存的代码（与 AutoSaver 保存格式一致）
    QString filePath = QString("data/user_answers/%1.cpp").arg(questionId);
    QFile file(filePath);
    
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString code = QString::fromUtf8(file.readAll());
        file.close();
        
        if (!code.isEmpty()) {
            qDebug() << "[MainWindow] Loaded saved code for question:" << questionId << "length:" << code.length();
            return code;
        }
    }
    
    qDebug() << "[MainWindow] No saved code found for question:" << questionId;
    return QString();
}

void MainWindow::loadCurrentQuestion()
{
    // 先保存当前题目的代码
    if (m_codeEditor) {
        qDebug() << "[MainWindow] Saving current code before switching question";
        m_codeEditor->forceSave();
    }
    
    if (m_currentQuestionIndex < 0 || m_currentQuestionIndex >= m_questionBank->count()) {
        return;
    }
    
    Question question = m_questionBank->allQuestions()[m_currentQuestionIndex];
    
    qDebug() << "[MainWindow] Loading question:" << question.id() << question.title();
    
    // 显示题目
    m_questionPanel->setQuestion(question);
    
    // 设置编辑器
    m_codeEditor->setQuestionId(question.id());
    
    // 尝试加载之前保存的代码
    loadSavedCode(question.id());
    
    // 更新AI助手的题目上下文
    if (m_aiAssistantPanel) {
        qDebug() << "[MainWindow] Calling setQuestionContext for question:" << question.id();
        m_aiAssistantPanel->setQuestionContext(question);
    } else {
        qWarning() << "[MainWindow] m_aiAssistantPanel is null!";
    }
    
    // 更新窗口标题
    setWindowTitle(QString("代码刷题系统 - %1 (%2/%3)")
        .arg(question.title())
        .arg(m_currentQuestionIndex + 1)
        .arg(m_questionBank->count()));
}

void MainWindow::loadSavedCode(const QString &questionId)
{
    // 从 .cpp 文件加载保存的代码
    QString filePath = QString("data/user_answers/%1.cpp").arg(questionId);
    QFile file(filePath);
    
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString savedCode = QString::fromUtf8(file.readAll());
        file.close();
        
        if (!savedCode.isEmpty()) {
            m_codeEditor->setCode(savedCode);
            qDebug() << "[MainWindow] Loaded saved code from:" << filePath << "length:" << savedCode.length();
        } else {
            // 文件为空，生成默认代码
            if (m_currentQuestionIndex >= 0 && m_currentQuestionIndex < m_questionBank->count()) {
                Question currentQuestion = m_questionBank->allQuestions()[m_currentQuestionIndex];
                m_codeEditor->setCode(generateDefaultCode(currentQuestion));
            } else {
                m_codeEditor->setCode("");
            }
        }
    } else {
        // 没有保存的代码文件，生成默认模板
        qDebug() << "[MainWindow] No saved code file found:" << filePath;
        if (m_currentQuestionIndex >= 0 && m_currentQuestionIndex < m_questionBank->count()) {
            Question currentQuestion = m_questionBank->allQuestions()[m_currentQuestionIndex];
            m_codeEditor->setCode(generateDefaultCode(currentQuestion));
        } else {
            m_codeEditor->setCode("");
        }
    }
}

void MainWindow::saveQuestionBank()
{
    // 保存到基础题库（分层结构）
    if (m_currentBankPath.isEmpty()) {
        return;
    }
    
    // 按源文件分组保存题目
    QMap<QString, QVector<Question>> questionsByFile;
    for (const auto &q : m_questionBank->allQuestions()) {
        // 从ID中提取源文件名（格式：{sourceFile}_{hash}）
        QString sourceFile = q.id().section('_', 0, 0);
        if (sourceFile.isEmpty()) {
            sourceFile = "未分类";
        }
        questionsByFile[sourceFile].append(q);
    }
    
    QDir dir;
    int savedCount = 0;
    
    for (auto it = questionsByFile.begin(); it != questionsByFile.end(); ++it) {
        QString sourceFileName = it.key();
        const QVector<Question> &questions = it.value();
        
        // 创建源文件对应的子目录
        QString subDir = QString("%1/%2").arg(m_currentBankPath).arg(sourceFileName);
        if (!dir.mkpath(subDir)) {
            qWarning() << "无法创建子目录:" << subDir;
            continue;
        }
        
        // 保存该文件的所有题目
        for (const Question &q : questions) {
            // 清理题目标题，用作文件名
            QString safeTitle = q.title();
            safeTitle.replace(QRegularExpression("[\\\\/:*?\"<>|]"), "_");
            safeTitle = safeTitle.trimmed();
            if (safeTitle.isEmpty()) {
                safeTitle = QString("题目%1").arg(savedCount + 1);
            }
            
            QString questionFilePath = QString("%1/%2.json").arg(subDir).arg(safeTitle);
            
            QFile file(questionFilePath);
            if (file.open(QIODevice::WriteOnly)) {
                QJsonDocument doc(q.toJson());
                file.write(doc.toJson(QJsonDocument::Indented));
                file.close();
                savedCount++;
            }
        }
    }
    
    qDebug() << "题库已保存到基础题库:" << m_currentBankPath << "共" << savedCount << "道题目";
}

QString MainWindow::generateDefaultCode(const Question &question)
{
    // 生成带基本输入输出框架的代码模板
    QString code = R"(#include <iostream>
using namespace std;

int main() {
    // TODO: 读取输入
    // 提示：根据题目要求读取输入数据
    
    // TODO: 处理逻辑
    // 提示：在这里实现题目要求的算法
    
    // TODO: 输出结果
    // 提示：按照题目要求的格式输出结果
    
    return 0;
}
)";
    
    return code;
}

void MainWindow::showTestResults(const QVector<TestResult> &results)
{
    int passed = 0;
    int total = results.size();
    
    // 统计通过的测试用例
    for (const auto &result : results) {
        if (result.passed) passed++;
    }
    
    bool allPassed = (passed == total && total > 0);
    
    QString resultText = R"(
        <style>
            .status-header { 
                background: #1e1e1e; 
                padding: 20px; 
                border-radius: 10px; 
                margin-bottom: 15px;
                text-align: center;
            }
            .accepted { 
                color: #00ff00; 
                font-size: 28px; 
                font-weight: bold;
            }
            .wrong-answer { 
                color: #ff4444; 
                font-size: 28px; 
                font-weight: bold;
            }
            .stats { 
                color: #b0b0b0; 
                font-size: 14px; 
                margin-top: 10px;
            }
            .test-case { 
                background: #1e1e1e; 
                padding: 12px; 
                margin: 8px 0; 
                border-left: 4px solid #3a3a3a;
                border-radius: 5px;
            }
            .test-pass { border-left-color: #00ff00; }
            .test-fail { border-left-color: #ff4444; }
            .test-title { 
                font-weight: bold; 
                margin-bottom: 8px;
                font-size: 11pt;
            }
            .test-detail { 
                font-family: 'Consolas', 'Monaco', monospace; 
                background: #242424; 
                padding: 8px; 
                margin: 5px 0;
                border-radius: 4px;
                font-size: 9pt;
                line-height: 1.4;
            }
            .label { 
                color: #888; 
                font-weight: bold;
                display: inline-block;
                min-width: 80px;
            }
            .value-correct { color: #00ff00; }
            .value-wrong { color: #ff4444; }
            .divider {
                border-top: 1px solid #3a3a3a;
                margin: 15px 0;
            }
        </style>
    )";
    
    // 状态头部（类似LeetCode）
    resultText += "<div class='status-header'>";
    if (allPassed) {
        resultText += "<div class='accepted'>✅ Accepted</div>";
        resultText += QString("<div class='stats'>所有测试用例通过 (%1/%2)</div>").arg(passed).arg(total);
    } else {
        resultText += "<div class='wrong-answer'>❌ Wrong Answer</div>";
        resultText += QString("<div class='stats'>通过 %1/%2 个测试用例</div>").arg(passed).arg(total);
    }
    resultText += "</div>";
    
    // 详细测试结果
    for (int i = 0; i < results.size(); ++i) {
        const TestResult &result = results[i];
        QString cssClass = result.passed ? "test-pass" : "test-fail";
        QString icon = result.passed ? "✅" : "❌";
        
        resultText += QString("<div class='test-case %1'>").arg(cssClass);
        
        // 标题行
        QString titleText = QString("测试用例 %1/%2").arg(i + 1).arg(total);
        if (!result.description.isEmpty()) {
            titleText += QString(" - %1").arg(result.description);
        }
        resultText += QString("<div class='test-title'>%1 %2</div>").arg(icon).arg(titleText);
        
        // 输入
        resultText += "<div class='test-detail'>";
        resultText += "<span class='label'>输入：</span>";
        resultText += QString("<span>%1</span>").arg(result.input.toHtmlEscaped());
        resultText += "</div>";
        
        // 期望输出
        resultText += "<div class='test-detail'>";
        resultText += "<span class='label'>期望输出：</span>";
        resultText += QString("<span class='value-correct'>%1</span>").arg(result.expectedOutput.toHtmlEscaped());
        resultText += "</div>";
        
        // 实际输出
        if (!result.passed) {
            resultText += "<div class='test-detail'>";
            resultText += "<span class='label'>实际输出：</span>";
            resultText += QString("<span class='value-wrong'>%1</span>").arg(result.actualOutput.toHtmlEscaped());
            resultText += "</div>";
            
            // 失败原因
            resultText += "<div class='test-detail'>";
            resultText += "<span class='label'>❗ 失败原因：</span>";
            QString reasonText;
            switch (result.failureReason) {
                case TestFailureReason::WrongAnswer:
                    reasonText = "答案错误";
                    break;
                case TestFailureReason::RuntimeError:
                    reasonText = "运行时错误";
                    break;
                case TestFailureReason::TimeLimitExceeded:
                    reasonText = "超时";
                    break;
                case TestFailureReason::MemoryLimitExceeded:
                    reasonText = "内存超限";
                    break;
                case TestFailureReason::CompileError:
                    reasonText = "编译错误";
                    break;
                default:
                    reasonText = "未知错误";
            }
            resultText += QString("<span style='color:#ff8800; font-weight:bold;'>%1</span>").arg(reasonText);
            resultText += "</div>";
            
            // 错误信息
            if (!result.error.isEmpty()) {
                resultText += "<div class='test-detail'>";
                resultText += "<span class='label'>错误信息：</span>";
                resultText += QString("<span style='color:#ff8800'>%1</span>").arg(result.error.toHtmlEscaped());
                resultText += "</div>";
            }
        } else {
            resultText += "<div class='test-detail'>";
            resultText += "<span class='label'>实际输出：</span>";
            resultText += QString("<span class='value-correct'>%1</span>").arg(result.actualOutput.toHtmlEscaped());
            resultText += "</div>";
        }
        
        // 执行时间
        resultText += "<div class='test-detail'>";
        resultText += "<span class='label'>⏱️ 执行时间：</span>";
        QString timeColor = result.executionTime > 1000 ? "#ff8800" : "#00ff00";
        resultText += QString("<span style='color:%1'>%2 ms</span>").arg(timeColor).arg(result.executionTime);
        resultText += "</div>";
        
        // 测试数据来源标注
        if (result.isAIGenerated) {
            resultText += "<div class='test-detail'>";
            resultText += "<span style='color:#888; font-size:8pt;'>🤖 AI补充测试数据</span>";
            resultText += "</div>";
        } else {
            resultText += "<div class='test-detail'>";
            resultText += "<span style='color:#888; font-size:8pt;'>📋 原始测试数据</span>";
            resultText += "</div>";
        }
        
        resultText += "</div>";
    }
    
    // 底部提示
    if (!allPassed) {
        resultText += "<div class='divider'></div>";
        resultText += "<div style='color:#b0b0b0; font-size:10pt; padding:10px;'>";
        resultText += "💡 <b>提示：</b>检查失败的测试用例，确保代码能正确处理所有情况。";
        resultText += "</div>";
    }
    
    // 显示测试结果对话框
    QDialog *resultDialog = new QDialog(this);
    resultDialog->setWindowTitle(allPassed ? "✅ Accepted" : "❌ Wrong Answer");
    resultDialog->setMinimumSize(700, 500);
    
    QVBoxLayout *layout = new QVBoxLayout(resultDialog);
    
    QTextEdit *resultView = new QTextEdit(resultDialog);
    resultView->setReadOnly(true);
    resultView->setHtml(resultText);
    layout->addWidget(resultView);
    
    QHBoxLayout *btnLayout = new QHBoxLayout();
    QPushButton *closeBtn = new QPushButton("关闭", resultDialog);
    closeBtn->setFixedWidth(100);
    connect(closeBtn, &QPushButton::clicked, resultDialog, &QDialog::accept);
    
    btnLayout->addStretch();
    btnLayout->addWidget(closeBtn);
    layout->addLayout(btnLayout);
    
    resultDialog->setStyleSheet(R"(
        QDialog {
            background-color: #242424;
        }
        QTextEdit {
            background-color: #1e1e1e;
            color: #e8e8e8;
            border: 1px solid #3a3a3a;
            border-radius: 5px;
        }
        QPushButton {
            background-color: #660000;
            color: white;
            border: none;
            border-radius: 8px;
            padding: 10px 24px;
            font-weight: 500;
        }
        QPushButton:hover {
            background-color: #880000;
        }
    )");
    
    resultDialog->exec();
    resultDialog->deleteLater();
    
    // 更新刷题进度
    if (m_currentQuestionIndex >= 0 && m_currentQuestionIndex < m_questionBank->count()) {
        Question currentQuestion = m_questionBank->allQuestions()[m_currentQuestionIndex];
        QString code = m_codeEditor->code();
        bool allPassed = (passed == total && total > 0);
        
        ProgressManager::instance().recordAttempt(currentQuestion.id(), allPassed, code);
        
        // 保存代码版本（无论通过与否）
        m_codeEditor->autoSaver()->saveVersion(allPassed, passed, total);
        
        // 如果失败，记录到错题本
        if (!allPassed && total > 0) {
            WrongQuestionBook::instance().addWrongQuestion(
                currentQuestion,
                code,
                QString("测试未通过 (%1/%2)").arg(passed).arg(total)
            );
        }
    }
    
    // 显示结果弹窗
    if (allPassed) {
        // Accepted!
        QMessageBox msgBox(this);
        msgBox.setWindowTitle("Accepted");
        msgBox.setText("🎉 Accepted!");
        msgBox.setInformativeText(QString("恭喜！所有 %1 个测试用例全部通过！\n\n"
                                         "你已经成功完成了这道题目。\n"
                                         "继续保持，加油！").arg(total));
        msgBox.setIcon(QMessageBox::Information);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setStyleSheet(R"(
            QMessageBox {
                background-color: #242424;
            }
            QMessageBox QLabel {
                color: #e8e8e8;
                font-size: 10pt;
            }
            QPushButton {
                background-color: #00aa00;
                color: white;
                border: none;
                border-radius: 8px;
                padding: 10px 24px;
                font-weight: 500;
                min-width: 80px;
            }
            QPushButton:hover {
                background-color: #00cc00;
            }
        )");
        msgBox.exec();
    } else {
        // Wrong Answer
        QMessageBox msgBox(this);
        msgBox.setWindowTitle("Wrong Answer");
        msgBox.setText("❌ Wrong Answer");
        msgBox.setInformativeText(QString("通过了 %1/%2 个测试用例\n\n"
                                         "请检查失败的测试用例，\n"
                                         "确保代码能正确处理所有情况。").arg(passed).arg(total));
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setStandardButtons(QMessageBox::Ok);
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
                padding: 10px 24px;
                font-weight: 500;
                min-width: 80px;
            }
            QPushButton:hover {
                background-color: #880000;
            }
        )");
        msgBox.exec();
        
        // 添加到错题本
        if (m_currentQuestionIndex >= 0 && m_currentQuestionIndex < m_questionBank->count()) {
            Question currentQuestion = m_questionBank->allQuestions()[m_currentQuestionIndex];
            QString errorReason = QString("Wrong Answer (%1/%2)").arg(passed).arg(total);
            WrongQuestionBook::instance().addWrongQuestion(currentQuestion, m_codeEditor->code(), errorReason);
        }
        
        // AI主动询问是否需要帮助
        if (m_aiAssistantPanel) {
            m_aiAssistantPanel->offerHelp(QString("我注意到测试没有全部通过（%1/%2）。\n\n"
                                        "需要我帮你分析一下吗？或者你想先自己思考一下？").arg(passed).arg(total));
        }
    }
}

void MainWindow::checkAIConnection()
{
    ConfigManager &config = ConfigManager::instance();
    
    // 创建连接检查器
    AIConnectionChecker *checker = new AIConnectionChecker(this);
    
    // 连接信号
    connect(checker, &AIConnectionChecker::allChecksCompleted, this, 
            &MainWindow::showAIConnectionStatus);
    
    // 显示检查提示
    statusBar()->showMessage("正在检查AI服务连接...", 0);
    
    // 开始检查
    QString ollamaUrl = config.ollamaUrl();
    QString ollamaModel = config.ollamaModel();
    QString cloudApiKey = config.cloudApiKey();
    QString cloudApiUrl = "https://api.openai.com/v1/chat/completions";
    
    if (ollamaUrl.isEmpty()) {
        ollamaUrl = "http://localhost:11434";
    }
    if (ollamaModel.isEmpty()) {
        ollamaModel = "qwen";
    }
    
    // 检查Ollama连接（总是检查）
    checker->checkOllamaConnection(ollamaUrl, ollamaModel);
    
    // 检查云端API（总是检查，即使没有配置）
    // 这样可以确保 m_pendingChecks 计数正确
    checker->checkCloudApiConnection(cloudApiKey, cloudApiUrl);
}

void MainWindow::showAIConnectionStatus(const AIConnectionStatus &status)
{
    // 保存状态供后续使用
    m_lastAIStatus = status;
    
    // 判断是否需要显示配置对话框
    bool needConfig = false;
    QString statusMessage;
    
    // 获取用户当前选择的模式
    ConfigManager &config = ConfigManager::instance();
    bool useCloudApi = config.useCloudApi();
    
    // 根据用户选择的模式显示对应的状态
    if (useCloudApi) {
        // 用户选择了云端API模式
        if (status.cloudApiAvailable) {
            statusMessage = "✅ AI服务已连接 - 云端API";
            statusBar()->showMessage(statusMessage, 5000);
            qInfo() << "AI连接检测：云端API可用（当前模式）";
        } else {
            // 云端API不可用
            needConfig = true;
            statusMessage = "⚠️ 云端API未配置或连接失败";
            statusBar()->showMessage(statusMessage, 0);
            qWarning() << "AI连接检测：云端API不可用（当前模式）";
            qWarning() << "  错误:" << status.cloudApiError;
        }
    } else {
        // 用户选择了本地Ollama模式
        if (status.ollamaAvailable) {
            statusMessage = QString("✅ AI服务已连接 - Ollama (%1)").arg(status.ollamaModel);
            statusBar()->showMessage(statusMessage, 5000);
            qInfo() << "AI连接检测：Ollama可用（当前模式） -" << status.ollamaModel;
        } else {
            // Ollama不可用
            needConfig = true;
            
            // 根据具体情况显示不同的提示
            if (status.needModelSelection && !status.availableModels.isEmpty()) {
                statusMessage = QString("⚠️ 配置的AI模型不可用，但检测到 %1 个其他模型")
                    .arg(status.availableModels.size());
            } else {
                statusMessage = "⚠️ Ollama未连接或未配置";
            }
            
            statusBar()->showMessage(statusMessage, 0);
            qWarning() << "AI连接检测：Ollama不可用（当前模式）";
            qWarning() << "  错误:" << status.ollamaError;
        }
    }
    
    // 只在需要配置时才弹窗
    if (needConfig) {
        QTimer::singleShot(100, this, [this, status]() {
            showAIConfigDialog(status);
        });
    }
}

void MainWindow::showAIConfigDialog(const AIConnectionStatus &status)
{
    ConfigManager &config = ConfigManager::instance();
    QString currentModel = config.ollamaModel();
    QString currentApiKey = config.cloudApiKey();
    
    // 使用传入的status中的可用模型列表
    QStringList availableModels = status.availableModels;
    
    // 构建对话框标题和提示信息
    QString dialogTitle = "🤖 AI服务配置";
    QString infoMessage;
    
    if (status.needModelSelection && !availableModels.isEmpty()) {
        dialogTitle = "⚠️ 需要选择AI模型";
        infoMessage = QString("检测到 %1 个可用模型，但配置的模型不可用。\n请选择一个模型继续使用。")
            .arg(availableModels.size());
    } else if (!status.ollamaError.isEmpty() && !status.cloudApiError.isEmpty()) {
        dialogTitle = "⚠️ AI服务未连接";
        infoMessage = "未检测到可用的AI服务。\n请配置本地Ollama或云端API。";
    } else {
        infoMessage = "请配置AI服务以使用智能功能。";
    }
    
    // 创建配置对话框
    QDialog dialog(this);
    dialog.setWindowTitle(dialogTitle);
    dialog.setMinimumWidth(600);
    dialog.setMinimumHeight(500);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(&dialog);
    
    // 信息提示
    if (!infoMessage.isEmpty()) {
        QLabel *infoLabel = new QLabel(infoMessage, &dialog);
        infoLabel->setStyleSheet("font-size: 11pt; color: #ff8800; padding: 10px; background: #2a1a00; border-radius: 5px; margin-bottom: 10px;");
        infoLabel->setWordWrap(true);
        mainLayout->addWidget(infoLabel);
    }
    
    // 标题
    QLabel *titleLabel = new QLabel("请配置AI服务", &dialog);
    titleLabel->setStyleSheet("font-size: 14pt; font-weight: bold; color: #e8e8e8;");
    mainLayout->addWidget(titleLabel);
    
    // 选项卡
    QTabWidget *tabWidget = new QTabWidget(&dialog);
    tabWidget->setStyleSheet(R"(
        QTabWidget::pane {
            border: 2px solid #3a3a3a;
            border-radius: 8px;
            background: #1e1e1e;
        }
        QTabBar::tab {
            background: #2a2a2a;
            color: #b0b0b0;
            padding: 10px 20px;
            border-top-left-radius: 8px;
            border-top-right-radius: 8px;
            margin-right: 2px;
        }
        QTabBar::tab:selected {
            background: #660000;
            color: white;
        }
        QTabBar::tab:hover {
            background: #3a3a3a;
        }
    )");
    
    // === 本地Ollama标签页 ===
    QWidget *localTab = new QWidget();
    QVBoxLayout *localLayout = new QVBoxLayout(localTab);
    
    QLabel *localInfo = new QLabel(
        "💻 使用本地Ollama服务\n"
        "• 完全免费，数据隐私\n"
        "• 需要先安装Ollama并下载模型",
        localTab
    );
    localInfo->setStyleSheet("color: #b0b0b0; padding: 10px; background: #1a1a1a; border-radius: 5px;");
    localLayout->addWidget(localInfo);
    
    QListWidget *modelList = nullptr;
    if (!availableModels.isEmpty()) {
        QLabel *modelLabel = new QLabel("检测到以下可用模型：", localTab);
        modelLabel->setStyleSheet("color: #e8e8e8; font-weight: bold; margin-top: 10px;");
        localLayout->addWidget(modelLabel);
        
        modelList = new QListWidget(localTab);
        modelList->setStyleSheet(R"(
            QListWidget {
                background-color: #1a1a1a;
                color: #e8e8e8;
                border: 2px solid #3a3a3a;
                border-radius: 8px;
                padding: 5px;
            }
            QListWidget::item {
                padding: 8px;
                border-radius: 5px;
            }
            QListWidget::item:selected {
                background-color: #660000;
                color: white;
            }
        )");
        
        for (const QString &model : availableModels) {
            modelList->addItem(model);
        }
        if (modelList->count() > 0) {
            modelList->setCurrentRow(0);
        }
        
        localLayout->addWidget(modelList);
    } else {
        QLabel *noModelLabel = new QLabel(
            "⚠️ 未检测到Ollama模型\n\n"
            "请先安装Ollama并下载模型：\n"
            "1. 访问 https://ollama.ai 下载安装\n"
            "2. 运行命令：ollama pull qwen2.5:7b\n"
            "3. 重启本程序",
            localTab
        );
        noModelLabel->setStyleSheet("color: #ff8800; padding: 20px; background: #2a1a00; border-radius: 8px;");
        localLayout->addWidget(noModelLabel);
    }
    
    localLayout->addStretch();
    
    // === 云端API标签页 ===
    QWidget *cloudTab = new QWidget();
    QVBoxLayout *cloudLayout = new QVBoxLayout(cloudTab);
    
    QLabel *cloudInfo = new QLabel(
        "☁️ 使用云端AI服务\n"
        "• 支持OpenAI、DeepSeek等API\n"
        "• 需要API Key（可能需要付费）",
        cloudTab
    );
    cloudInfo->setStyleSheet("color: #b0b0b0; padding: 10px; background: #1a1a1a; border-radius: 5px;");
    cloudLayout->addWidget(cloudInfo);
    
    QLabel *apiKeyLabel = new QLabel("API Key:", cloudTab);
    apiKeyLabel->setStyleSheet("color: #e8e8e8; font-weight: bold; margin-top: 15px;");
    cloudLayout->addWidget(apiKeyLabel);
    
    QLineEdit *apiKeyEdit = new QLineEdit(cloudTab);
    apiKeyEdit->setPlaceholderText("输入你的API Key...");
    apiKeyEdit->setText(currentApiKey);
    apiKeyEdit->setEchoMode(QLineEdit::Password);
    apiKeyEdit->setStyleSheet(R"(
        QLineEdit {
            background-color: #1a1a1a;
            color: #e8e8e8;
            border: 2px solid #3a3a3a;
            border-radius: 8px;
            padding: 10px;
            font-size: 10pt;
        }
        QLineEdit:focus {
            border-color: #660000;
        }
    )");
    cloudLayout->addWidget(apiKeyEdit);
    
    QLabel *tipLabel = new QLabel(
        "💡 提示：\n"
        "• OpenAI: 使用默认地址\n"
        "• DeepSeek等兼容OpenAI API的服务也可使用",
        cloudTab
    );
    tipLabel->setStyleSheet("color: #888; font-size: 9pt; margin-top: 10px;");
    cloudLayout->addWidget(tipLabel);
    
    cloudLayout->addStretch();
    
    tabWidget->addTab(localTab, "🖥️ 本地Ollama");
    tabWidget->addTab(cloudTab, "☁️ 云端API");
    
    // 根据当前配置选择标签页
    if (config.useCloudMode()) {
        tabWidget->setCurrentIndex(1);  // 云端标签页
    } else {
        tabWidget->setCurrentIndex(0);  // 本地标签页
    }
    
    mainLayout->addWidget(tabWidget);
    
    // 按钮
    QHBoxLayout *btnLayout = new QHBoxLayout();
    QPushButton *okBtn = new QPushButton("✓ 确定", &dialog);
    QPushButton *skipBtn = new QPushButton("⏭️ 跳过", &dialog);
    
    QString btnStyle = R"(
        QPushButton {
            background-color: #660000;
            color: white;
            border: none;
            border-radius: 8px;
            padding: 12px 28px;
            font-weight: 600;
            font-size: 10pt;
            min-width: 100px;
        }
        QPushButton:hover {
            background-color: #880000;
        }
        QPushButton:pressed {
            background-color: #440000;
        }
    )";
    
    okBtn->setStyleSheet(btnStyle);
    skipBtn->setStyleSheet(btnStyle);
    
    connect(okBtn, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(skipBtn, &QPushButton::clicked, &dialog, &QDialog::reject);
    
    btnLayout->addStretch();
    btnLayout->addWidget(okBtn);
    btnLayout->addWidget(skipBtn);
    mainLayout->addLayout(btnLayout);
    
    dialog.setStyleSheet("QDialog { background-color: #242424; }");
    
    if (dialog.exec() == QDialog::Accepted) {
        int currentTab = tabWidget->currentIndex();
        
        if (currentTab == 0) {
            // 本地Ollama模式
            if (modelList && modelList->currentItem()) {
                QString selectedModel = modelList->currentItem()->text();
                
                // 保存配置：使用本地模型（不清空云端配置）
                config.setOllamaModel(selectedModel);
                config.setUseCloudMode(false);  // 设置当前使用本地模式
                config.save();
                
                // 立即切换到本地模式
                m_ollamaClient->setCloudMode(false);
                m_ollamaClient->setBaseUrl(config.ollamaUrl());
                m_ollamaClient->setModel(selectedModel);
                
                statusBar()->showMessage(QString("✓ 已配置本地模型：%1").arg(selectedModel), 5000);
                
                QMessageBox::information(this, "配置成功", 
                    QString("已切换到本地Ollama模式\n\n模型：%1\n\n现在可以使用AI功能了！").arg(selectedModel));
                
                qDebug() << "[MainWindow] 用户选择本地模式，模型:" << selectedModel;
            }
        } else {
            // 云端API模式
            QString apiKey = apiKeyEdit->text().trimmed();
            if (apiKey.isEmpty()) {
                QMessageBox::warning(this, "配置错误", "请输入有效的API Key");
                return;
            }
            
            // 保存配置：使用云端API（不清空本地配置）
            config.setCloudApiKey(apiKey);
            config.setUseCloudMode(true);  // 设置当前使用云端模式
            config.save();
            
            // 立即切换到云端模式
            m_ollamaClient->setCloudMode(true);
            m_ollamaClient->setApiKey(apiKey);
            
            statusBar()->showMessage("✓ 已配置云端API", 5000);
            
            QMessageBox::information(this, "配置成功", 
                "已切换到云端API模式\n\n现在可以使用AI功能了！\n\n注意：AI分析功能将使用云端API服务（DeepSeek）");
            
            qDebug() << "[MainWindow] 用户选择云端API模式";
        }
    }
}

void MainWindow::onDeleteQuestions(const QVector<int> &indices)
{
    if (indices.isEmpty()) {
        return;
    }
    
    // 确认删除
    QString message;
    if (indices.size() == 1) {
        Question q = m_questionBank->allQuestions()[indices[0]];
        message = QString("确定要删除题目吗？\n\n【%1】\n\n此操作不可撤销！").arg(q.title());
    } else {
        message = QString("确定要删除选中的 %1 道题目吗？\n\n此操作不可撤销！").arg(indices.size());
    }
    
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "确认删除", message,
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    
    if (reply != QMessageBox::Yes) {
        return;
    }
    
    // 按降序排序索引，从后往前删除，避免索引变化
    QVector<int> sortedIndices = indices;
    std::sort(sortedIndices.begin(), sortedIndices.end(), std::greater<int>());
    
    // 删除题目（同时删除对应的JSON文件）
    for (int index : sortedIndices) {
        if (index >= 0 && index < m_questionBank->count()) {
            // 获取题目信息，用于删除文件
            Question q = m_questionBank->allQuestions()[index];
            
            // 删除对应的JSON文件（支持分层结构）
            if (!m_currentBankPath.isEmpty()) {
                // 从ID中提取源文件名
                QString sourceFile = q.id().section('_', 0, 0);
                if (sourceFile.isEmpty()) {
                    sourceFile = "未分类";
                }
                
                QString safeTitle = q.title();
                safeTitle.replace(QRegularExpression("[\\\\/:*?\"<>|]"), "_");
                safeTitle = safeTitle.trimmed();
                QString questionFilePath = QString("%1/%2/%3.json")
                    .arg(m_currentBankPath)
                    .arg(sourceFile)
                    .arg(safeTitle);
                
                QFile::remove(questionFilePath);
            }
            
            // 从题库中删除
            m_questionBank->removeQuestion(index);
        }
    }
    
    // 更新UI
    m_questionBankPanel->refreshBankTree();
    
    // 调整当前题目索引
    if (m_currentQuestionIndex >= m_questionBank->count()) {
        m_currentQuestionIndex = m_questionBank->count() - 1;
    }
    
    if (m_currentQuestionIndex >= 0) {
        loadCurrentQuestion();
    }
    // 如果没有题目了，loadCurrentQuestion会处理
    
    // 注意：不需要再调用saveQuestionBank()，因为已经直接删除了文件
    
    statusBar()->showMessage(QString("✓ 已删除 %1 道题目").arg(indices.size()), 3000);
}

void MainWindow::onErrorClicked(int line, int column)
{
    // 跳转到错误位置
    m_codeEditor->setCursorPosition(line - 1, column - 1);  // QScintilla使用0基索引
    m_codeEditor->setFocus();
    
    // 确保行可见
    m_codeEditor->ensureLineVisible(line - 1);
}

void MainWindow::onSyntaxErrorsFound(const QVector<SyntaxError> &errors)
{
    qDebug() << "[MainWindow] onSyntaxErrorsFound called with" << errors.size() << "errors";
    
    // 更新错误列表
    m_errorListWidget->setErrors(errors);
    
    // 有错误时自动显示错误列表，无错误时隐藏
    if (!errors.isEmpty()) {
        qDebug() << "[MainWindow] Showing error list widget";
        m_errorListWidget->setVisible(true);
    } else {
        qDebug() << "[MainWindow] Hiding error list widget";
        m_errorListWidget->setVisible(false);
    }
}
