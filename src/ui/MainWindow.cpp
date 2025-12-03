#include "MainWindow.h"
#include "SmartImportDialog.h"
#include "QuestionBankManagerDialog.h"
#include "ExamGeneratorDialog.h"
#include "MockExamManagerDialog.h"
#include "HistoryWidget.h"
#include "QuestionListWidget.h"
#include "WrongQuestionWidget.h"
#include "PracticeWidget.h"
#include "SettingsDialog.h"
#include "OriginalQuestionDialog.h"
#include "CodeVersionDialog.h"
#include "ErrorListWidget.h"
#include "StyleManager.h"
#include "../core/QuestionBankManager.h"
#include "../utils/AIConnectionChecker.h"
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
#include <QToolBar>
#include <QDockWidget>
#include <QInputDialog>
#include <QCloseEvent>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_currentQuestionIndex(-1)
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
    
    // 应用题目列表样式
    m_questionListWidget->setStyleSheet(StyleManager::getQuestionListStyle());
    
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
    
    // 创建题目列表侧边栏
    m_questionListWidget = new QuestionListWidget(this);
    QDockWidget *questionListDock = new QDockWidget("题目列表", this);
    questionListDock->setWidget(m_questionListWidget);
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
    
    toolBar->addSeparator();
    
    // 错误列表切换按钮
    QAction *toggleErrorsAction = toolBar->addAction("🐛 错误列表");
    toggleErrorsAction->setCheckable(true);
    toggleErrorsAction->setChecked(false);
    toggleErrorsAction->setShortcut(QKeySequence("Ctrl+Shift+M"));
    toggleErrorsAction->setToolTip("显示/隐藏错误列表 (Ctrl+Shift+M)");
    toggleErrorsAction->setStatusTip("切换错误列表面板的显示状态");
    connect(toggleErrorsAction, &QAction::triggered, this, [this](bool checked) {
        m_errorListWidget->setVisible(checked);
    });
    
    // 注意：上一题/下一题、运行测试、AI分析等按钮已在刷题模式界面中提供
    // 这里不再重复添加，保持工具栏简洁
}

void MainWindow::setupMenuBar()
{
    // 文件菜单
    QMenu *fileMenu = menuBar()->addMenu("文件(&F)");
    
    QAction *importAction = fileMenu->addAction("🤖 AI智能导入题库(&I)...");
    importAction->setShortcut(QKeySequence("Ctrl+I"));
    importAction->setStatusTip("AI自动识别格式、解析题目、生成测试数据");
    connect(importAction, &QAction::triggered, this, &MainWindow::onImportQuestionBank);
    
    QAction *refreshAction = fileMenu->addAction("刷新题库(&R)");
    refreshAction->setShortcut(QKeySequence("Ctrl+F5"));
    refreshAction->setStatusTip("重新加载当前题库");
    connect(refreshAction, &QAction::triggered, this, &MainWindow::onRefreshQuestionBank);
    
    // 移除重复的"重新加载题库"功能，已合并到"刷新题库"
    // QAction *reloadAction = fileMenu->addAction("重新加载题库(&L)...");
    // connect(reloadAction, &QAction::triggered, this, &MainWindow::onReloadQuestionBank);
    
    QAction *manageAction = fileMenu->addAction("题库管理(&M)...");
    manageAction->setShortcut(QKeySequence("Ctrl+M"));
    manageAction->setStatusTip("管理所有题库，切换、删除或查看题库信息");
    connect(manageAction, &QAction::triggered, this, &MainWindow::onManageQuestionBanks);
    
    QAction *clearAction = fileMenu->addAction("清空当前题库(&C)...");
    clearAction->setShortcut(QKeySequence("Ctrl+Shift+C"));
    clearAction->setStatusTip("清空当前加载的题库（不删除文件）");
    connect(clearAction, &QAction::triggered, this, &MainWindow::onClearQuestionBank);
    
    fileMenu->addSeparator();
    
    QAction *exitAction = fileMenu->addAction("退出(&X)");
    exitAction->setShortcut(QKeySequence("Ctrl+Q"));
    connect(exitAction, &QAction::triggered, this, &QWidget::close);
    
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
        
        // 切换到刷题模式
        m_stackedWidget->setCurrentIndex(0);
        
        // 在题库中找到题目索引
        bool found = false;
        for (int i = 0; i < m_questionBank->count(); ++i) {
            if (m_questionBank->allQuestions()[i].id() == question.id()) {
                m_currentQuestionIndex = i;
                
                // 安全地设置题目
                if (m_questionPanel) {
                    m_questionPanel->setQuestion(question);
                }
                
                // 加载保存的代码
                loadSavedCode(question.id());
                
                // 更新题目列表选中状态
                if (m_questionListWidget) {
                    m_questionListWidget->setCurrentQuestion(i);
                }
                
                statusBar()->showMessage(QString("已选择题目: %1").arg(question.title()), 3000);
                found = true;
                break;
            }
        }
        
        if (!found) {
            qWarning() << "[MainWindow] Question not found in bank:" << question.id();
            statusBar()->showMessage("题目未找到", 3000);
        }
    });
    
    // 题目面板信号
    connect(m_questionPanel, &QuestionPanel::runTests, 
            this, &MainWindow::onRunTests);
    connect(m_questionPanel, &QuestionPanel::nextQuestion, 
            this, &MainWindow::onNextQuestion);
    connect(m_questionPanel, &QuestionPanel::previousQuestion, 
            this, &MainWindow::onPreviousQuestion);
    
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
    
    // 题目列表信号
    connect(m_questionListWidget, &QuestionListWidget::questionSelected,
            this, &MainWindow::onQuestionSelectedFromList);
    connect(m_questionListWidget, &QuestionListWidget::questionsDeleteRequested,
            this, &MainWindow::onDeleteQuestions);
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
        m_ollamaClient->setApiKey(config.cloudApiKey());
        qDebug() << "[MainWindow] 配置为云端API模式";
    } else {
        // 使用本地Ollama
        m_ollamaClient->setCloudMode(false);
        m_ollamaClient->setBaseUrl(config.ollamaUrl());
        m_ollamaClient->setModel(config.ollamaModel());
        qDebug() << "[MainWindow] 配置为本地Ollama模式，模型:" << config.ollamaModel();
    }
}

void MainWindow::loadLastSession()
{
    QString questionBankPath;
    int questionIndex;
    
    if (SessionManager::instance().loadSession(questionBankPath, questionIndex)) {
        // 检查题库文件是否存在
        if (QFile::exists(questionBankPath)) {
            m_questionBank->loadFromDirectory(questionBankPath);
            
            if (m_questionBank->count() > 0) {
                m_currentQuestionIndex = qBound(0, questionIndex, m_questionBank->count() - 1);
                loadCurrentQuestion();
                
                statusBar()->showMessage(
                    QString("已恢复上次会话：%1 道题目，当前第 %2 题")
                    .arg(m_questionBank->count())
                    .arg(m_currentQuestionIndex + 1), 5000);
            }
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
    // 保存会话
    if (m_questionBank->count() > 0) {
        SessionManager::instance().saveSession("data/questions", m_currentQuestionIndex);
    }
    
    // 保存窗口状态
    SessionManager::instance().saveWindowState(saveGeometry(), saveState());
    
    event->accept();
}

void MainWindow::onImportQuestionBank()
{
    // 只支持AI智能导入
    
    // 选择题库文件夹
    QString path = QFileDialog::getExistingDirectory(
        this,
        "选择题库文件夹",
        QDir::homePath(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
    );
    
    if (path.isEmpty()) {
        return;
    }
    
    // 询问题库名称（分类名称）
    bool ok;
    QString categoryName = QInputDialog::getText(
        this, 
        "题库分类",
        "请输入题库分类名称（如：ccf、leetcode）:",
        QLineEdit::Normal,
        QFileInfo(path).fileName(), 
        &ok
    );
    
    if (!ok || categoryName.isEmpty()) {
        return;
    }
    
    // 使用AI智能导入
    SmartImportDialog *smartDialog = new SmartImportDialog(path, categoryName, m_ollamaClient, this);
    if (smartDialog->exec() == QDialog::Accepted && smartDialog->isSuccess()) {
        // SmartQuestionImporter已经保存了所有数据：
        // 1. data/原始题库/{categoryName}/ - 只读备份
        // 2. data/基础题库/{categoryName}/ - 标准化题库
        // 3. data/config/ccf_parse_rule.json - 解析规则
        // 4. data/question_banks/{categoryName}/questions.json - 运行时题库
        
        // 从保存的JSON加载题库
        QString bankPath = QString("data/question_banks/%1").arg(categoryName);
        QString jsonPath = bankPath + "/questions.json";
        
        QFile jsonFile(jsonPath);
        if (jsonFile.open(QIODevice::ReadOnly)) {
            QJsonDocument doc = QJsonDocument::fromJson(jsonFile.readAll());
            jsonFile.close();
            
            if (doc.isArray()) {
                // 清空现有题库
                m_questionBank->clear();
                
                // 加载题目
                QJsonArray questionsArray = doc.array();
                for (const QJsonValue &val : questionsArray) {
                    m_questionBank->addQuestion(Question(val.toObject()));
                }
                
                // 更新UI
                m_questionListWidget->setQuestions(m_questionBank->allQuestions());
                
                if (m_questionBank->count() > 0) {
                    m_currentQuestionIndex = 0;
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
                                "• 基础题库：data/基础题库/%1/\n"
                                "• 解析规则：data/config/ccf_parse_rule.json\n"
                                "• 运行时题库：%6\n\n"
                                "✅ 现在可以直接刷题或生成模拟题！")
                        .arg(categoryName)
                        .arg(m_questionBank->count())
                        .arg(totalTestCases)
                        .arg(totalTestCases - aiGeneratedCases)
                        .arg(aiGeneratedCases)
                        .arg(jsonPath));
                }
            }
        } else {
            QMessageBox::warning(this, "加载失败", 
                QString("无法加载题库文件：%1").arg(jsonPath));
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
    connect(dialog, &QuestionBankManagerDialog::bankSelected, this, [this](const QString &bankId) {
        if (bankId.isEmpty()) {
            // 需要导入新题库
            QTimer::singleShot(100, this, &MainWindow::onImportQuestionBank);
        } else {
            // 切换到选中的题库
            QuestionBankInfo info = QuestionBankManager::instance().getBankInfo(bankId);
            QString bankPath = info.path;
            
            // 清空当前题库
            m_questionBank->clear();
            
            // 加载新题库
            m_questionBank->loadFromDirectory(bankPath);
            
            // 更新UI
            m_questionListWidget->setQuestions(m_questionBank->allQuestions());
            
            if (m_questionBank->count() > 0) {
                m_currentQuestionIndex = 0;
                loadCurrentQuestion();
                
                statusBar()->showMessage(
                    QString("已切换到题库：%1（%2 道题目）")
                    .arg(info.name).arg(m_questionBank->count()), 5000);
            }
            
            // 刷新题库列表
            m_practiceWidget->refreshQuestionList();
        }
    });
    
    connect(dialog, &QuestionBankManagerDialog::bankDeleted, this, [this](const QString &bankId) {
        // 如果删除的是当前题库，清空
        if (QuestionBankManager::instance().getCurrentBankId() == bankId) {
            m_questionBank->clear();
            m_currentQuestionIndex = -1;
            m_questionPanel->setQuestion(Question());
            m_codeEditor->setCode("");
            m_questionListWidget->setQuestions(QVector<Question>());
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
        m_questionListWidget->setQuestions(QVector<Question>());
        
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
                m_questionListWidget->setQuestions(m_questionBank->allQuestions());
                
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
    m_questionListWidget->setQuestions(m_questionBank->allQuestions());
}

void MainWindow::onQuestionSelectedFromList(int index)
{
    if (index >= 0 && index < m_questionBank->count()) {
        m_currentQuestionIndex = index;
        loadCurrentQuestion();
        m_questionListWidget->setCurrentQuestion(index);
    }
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
        m_aiAssistantPanel->setQuestionContext(question);
    }
    
    // 更新窗口标题
    setWindowTitle(QString("代码刷题系统 - %1 (%2/%3)")
        .arg(question.title())
        .arg(m_currentQuestionIndex + 1)
        .arg(m_questionBank->count()));
}

void MainWindow::loadSavedCode(const QString &questionId)
{
    QString filePath = QString("data/user_answers/%1.json").arg(questionId);
    QFile file(filePath);
    
    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        QJsonObject obj = doc.object();
        QString savedCode = obj["code"].toString();
        
        if (!savedCode.isEmpty()) {
            m_codeEditor->setCode(savedCode);
        } else {
            // 生成带输入输出框架的默认代码
            Question currentQuestion = m_questionBank->allQuestions()[m_currentQuestionIndex];
            m_codeEditor->setCode(generateDefaultCode(currentQuestion));
        }
        
        file.close();
    } else {
        // 没有保存的代码，生成默认模板
        Question currentQuestion = m_questionBank->allQuestions()[m_currentQuestionIndex];
        m_codeEditor->setCode(generateDefaultCode(currentQuestion));
    }
}

void MainWindow::saveQuestionBank()
{
    QDir dir("data/questions");
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    
    QJsonArray questionsArray;
    for (const auto &q : m_questionBank->allQuestions()) {
        questionsArray.append(q.toJson());
    }
    
    QFile file("data/questions/bank.json");
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(questionsArray).toJson());
        file.close();
    }
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
    
    // 检查Ollama连接
    checker->checkOllamaConnection(ollamaUrl, ollamaModel);
    
    // 如果配置了云端API，也检查
    if (!cloudApiKey.isEmpty()) {
        checker->checkCloudApiConnection(cloudApiKey, cloudApiUrl);
    }
}

void MainWindow::showAIConnectionStatus(const AIConnectionStatus &status)
{
    // 总是显示AI配置对话框
    QTimer::singleShot(100, this, &MainWindow::checkAndSelectModel);
    
    // 简单的状态栏提示
    if (status.ollamaAvailable) {
        statusBar()->showMessage(QString("✓ Ollama已连接 - %1").arg(status.ollamaModel), 5000);
    } else if (status.cloudApiAvailable) {
        statusBar()->showMessage("✓ 云端API已连接", 5000);
    } else {
        statusBar()->showMessage("⚠ AI服务未配置（不影响刷题功能）", 0);
    }
}

void MainWindow::checkAndSelectModel()
{
    ConfigManager &config = ConfigManager::instance();
    QString currentModel = config.ollamaModel();
    QString currentApiKey = config.cloudApiKey();
    
    // 临时保存当前模式
    bool wasCloudMode = m_ollamaClient->isCloudMode();
    
    // 临时切换到本地模式以检测可用模型
    if (wasCloudMode) {
        m_ollamaClient->setCloudMode(false);
        m_ollamaClient->setBaseUrl(config.ollamaUrl());
    }
    
    // 获取可用模型列表
    QStringList availableModels = m_ollamaClient->getAvailableModels();
    
    // 恢复之前的模式
    if (wasCloudMode) {
        m_ollamaClient->setCloudMode(true);
        m_ollamaClient->setApiKey(config.cloudApiKey());
    }
    
    // 总是显示配置对话框
    QDialog dialog(this);
    dialog.setWindowTitle("🤖 AI服务配置");
    dialog.setMinimumWidth(600);
    dialog.setMinimumHeight(500);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(&dialog);
    
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
    
    // 删除题目
    for (int index : sortedIndices) {
        if (index >= 0 && index < m_questionBank->count()) {
            m_questionBank->removeQuestion(index);
        }
    }
    
    // 更新UI
    m_questionListWidget->setQuestions(m_questionBank->allQuestions());
    
    // 调整当前题目索引
    if (m_currentQuestionIndex >= m_questionBank->count()) {
        m_currentQuestionIndex = m_questionBank->count() - 1;
    }
    
    if (m_currentQuestionIndex >= 0) {
        loadCurrentQuestion();
    }
    // 如果没有题目了，loadCurrentQuestion会处理
    
    // 保存题库
    saveQuestionBank();
    
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
