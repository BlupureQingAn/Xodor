#include "CodeEditor.h"
#include "../ai/OllamaClient.h"
#include <QVBoxLayout>
#include <QFont>
#include <QFontInfo>
#include <QKeyEvent>
#include <QDebug>
#include <Qsci/qscilexercpp.h>

CodeEditor::CodeEditor(QWidget *parent)
    : QWidget(parent)
    , m_syntaxChecker(nullptr)
    , m_aiClient(nullptr)
    , m_errorIndicator(0)
    , m_warningIndicator(1)
{
    qDebug() << "=== CodeEditor Constructor START ===";
    
    m_autoSaver = new AutoSaver(this);
    setupEditor();
    setupSyntaxChecker();
    setupErrorIndicators();
    
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(m_editor);
    layout->setContentsMargins(0, 0, 0, 0);
    
    // 安装事件过滤器以捕获键盘事件
    m_editor->installEventFilter(this);
    
    qDebug() << "=== CodeEditor Constructor END ===";
}

void CodeEditor::setupEditor()
{
    m_editor = new QsciScintilla(this);
    
    // === 字体设置 - 使用现代圆润字体 ===
    QFont font;
    #ifdef Q_OS_WIN
        // Windows优先使用Cascadia Code或Consolas
        QStringList fontFamilies = {"Cascadia Code", "Consolas", "Microsoft YaHei Mono", "Courier New"};
    #elif defined(Q_OS_MAC)
        QStringList fontFamilies = {"SF Mono", "Menlo", "Monaco"};
    #else
        QStringList fontFamilies = {"Fira Code", "Ubuntu Mono", "DejaVu Sans Mono", "Monospace"};
    #endif
    
    for (const QString &family : fontFamilies) {
        font.setFamily(family);
        if (QFontInfo(font).family() == family) {
            break;
        }
    }
    
    font.setPointSize(11);
    font.setStyleHint(QFont::Monospace);
    font.setFixedPitch(true);
    m_editor->setFont(font);
    
    // === C++语法高亮 - 深色主题配色 ===
    QsciLexerCPP *lexer = new QsciLexerCPP(m_editor);
    
    // 设置词法分析器字体
    lexer->setFont(font);
    
    // 深色主题配色方案
    lexer->setColor(QColor("#e8e8e8"), QsciLexerCPP::Default);           // 默认文本
    lexer->setColor(QColor("#6a9955"), QsciLexerCPP::Comment);           // 注释 - 绿色
    lexer->setColor(QColor("#6a9955"), QsciLexerCPP::CommentLine);       // 单行注释
    lexer->setColor(QColor("#6a9955"), QsciLexerCPP::CommentDoc);        // 文档注释
    lexer->setColor(QColor("#ce9178"), QsciLexerCPP::Number);            // 数字 - 橙色
    lexer->setColor(QColor("#c586c0"), QsciLexerCPP::Keyword);           // 关键字 - 紫色
    lexer->setColor(QColor("#ce9178"), QsciLexerCPP::DoubleQuotedString); // 字符串 - 橙色
    lexer->setColor(QColor("#ce9178"), QsciLexerCPP::SingleQuotedString); // 字符 - 橙色
    lexer->setColor(QColor("#4ec9b0"), QsciLexerCPP::PreProcessor);      // 预处理 - 青色
    lexer->setColor(QColor("#d4d4d4"), QsciLexerCPP::Operator);          // 操作符 - 浅灰
    lexer->setColor(QColor("#4fc1ff"), QsciLexerCPP::Identifier);        // 标识符 - 蓝色
    lexer->setColor(QColor("#dcdcaa"), QsciLexerCPP::UnclosedString);    // 未闭合字符串 - 黄色
    lexer->setColor(QColor("#569cd6"), QsciLexerCPP::KeywordSet2);       // 类型关键字 - 蓝色
    lexer->setColor(QColor("#4ec9b0"), QsciLexerCPP::GlobalClass);       // 全局类 - 青色
    
    // 设置关键字加粗
    QFont boldFont = font;
    boldFont.setBold(true);
    lexer->setFont(boldFont, QsciLexerCPP::Keyword);
    lexer->setFont(boldFont, QsciLexerCPP::KeywordSet2);
    
    m_editor->setLexer(lexer);
    
    // === 编辑器背景和前景色 ===
    m_editor->setPaper(QColor("#1e1e1e"));  // 背景 - 深灰黑
    m_editor->setColor(QColor("#e8e8e8"));  // 前景 - 浅灰白
    
    // === 行号边距 - 深色主题 ===
    m_editor->setMarginType(0, QsciScintilla::NumberMargin);
    m_editor->setMarginWidth(0, "000000");  // 6位数字宽度
    m_editor->setMarginsForegroundColor(QColor("#858585"));  // 行号颜色 - 中灰
    m_editor->setMarginsBackgroundColor(QColor("#242424"));  // 行号背景 - 深灰黑
    m_editor->setMarginLineNumbers(0, true);
    
    // === 当前行高亮 ===
    m_editor->setCaretLineVisible(true);
    m_editor->setCaretLineBackgroundColor(QColor("#2d2d2d"));  // 当前行背景
    m_editor->setCaretForegroundColor(QColor("#e8e8e8"));      // 光标颜色
    
    // === 选中文本样式 ===
    m_editor->setSelectionBackgroundColor(QColor("#660000"));  // 选中背景 - 深红
    m_editor->setSelectionForegroundColor(QColor("#ffffff"));  // 选中文本 - 白色
    
    // === 括号匹配 ===
    m_editor->setBraceMatching(QsciScintilla::SloppyBraceMatch);
    m_editor->setMatchedBraceBackgroundColor(QColor("#660000"));  // 匹配括号背景
    m_editor->setMatchedBraceForegroundColor(QColor("#ffffff"));  // 匹配括号前景
    m_editor->setUnmatchedBraceBackgroundColor(QColor("#880000")); // 不匹配括号背景
    m_editor->setUnmatchedBraceForegroundColor(QColor("#ff6b6b")); // 不匹配括号前景
    
    // === 缩进参考线 ===
    m_editor->setIndentationGuides(true);
    m_editor->setIndentationGuidesForegroundColor(QColor("#3a3a3a"));
    m_editor->setIndentationGuidesBackgroundColor(QColor("#242424"));
    
    // === 自动补全 ===
    m_editor->setAutoCompletionSource(QsciScintilla::AcsAll);
    m_editor->setAutoCompletionThreshold(2);
    m_editor->setAutoCompletionCaseSensitivity(false);
    m_editor->setAutoCompletionReplaceWord(true);
    
    // === 自动缩进 ===
    m_editor->setAutoIndent(true);
    m_editor->setTabWidth(4);
    m_editor->setIndentationsUseTabs(false);  // 使用空格而不是Tab
    m_editor->setTabIndents(true);
    m_editor->setBackspaceUnindents(true);
    
    // === 折叠 ===
    m_editor->setFolding(QsciScintilla::BoxedTreeFoldStyle);
    m_editor->setFoldMarginColors(QColor("#242424"), QColor("#242424"));
    
    // === 边距 ===
    m_editor->setMarginWidth(1, 0);  // 隐藏符号边距
    m_editor->setMarginWidth(2, 12); // 折叠边距
    
    // === 滚动条样式 ===
    m_editor->SendScintilla(QsciScintilla::SCI_SETSCROLLWIDTH, 1);
    m_editor->SendScintilla(QsciScintilla::SCI_SETSCROLLWIDTHTRACKING, true);
    
    // === 空白字符显示 ===
    m_editor->setWhitespaceVisibility(QsciScintilla::WsInvisible);
    
    // === EOL显示 ===
    m_editor->setEolVisibility(false);
    
    // QScintilla使用textChanged()槽而不是信号，需要使用SIGNAL/SLOT宏
    bool connected = connect(m_editor, SIGNAL(textChanged()),
                             this, SLOT(onTextChanged()));
    qDebug() << "[CodeEditor] textChanged signal connected:" << connected;
}

void CodeEditor::setCode(const QString &code)
{
    qDebug() << "[CodeEditor] setCode called, length:" << code.length();
    m_editor->setText(code);
    
    // 加载代码后立即触发语法检查
    if (m_syntaxChecker && !code.trimmed().isEmpty()) {
        qDebug() << "[CodeEditor] Triggering syntax check after setCode...";
        m_syntaxChecker->checkCode(code, "g++");
    }
}

QString CodeEditor::code() const
{
    return m_editor->text();
}

void CodeEditor::setQuestionId(const QString &id)
{
    m_currentQuestionId = id;
    m_autoSaver->setQuestionId(id);
    qDebug() << "[CodeEditor] Question ID set to:" << id;
}

void CodeEditor::onTextChanged()
{
    QString currentCode = m_editor->text();
    
    qDebug() << "[CodeEditor] Text changed, code length:" << currentCode.length();
    
    // 立即保存代码
    m_autoSaver->setContent(currentCode);
    m_autoSaver->triggerSave();
    
    // 发出代码变化信号
    emit codeChanged(currentCode);
    
    // 实时触发语法检查（延迟500ms）
    if (m_syntaxChecker) {
        if (!currentCode.trimmed().isEmpty()) {
            qDebug() << "[CodeEditor] Triggering syntax check...";
            m_syntaxChecker->checkCode(currentCode, "g++");
        } else {
            qDebug() << "[CodeEditor] Code is empty, clearing errors";
            emit syntaxErrorsFound(QVector<SyntaxError>());
        }
    } else {
        qDebug() << "[CodeEditor] WARNING: Syntax checker is null!";
    }
}

void CodeEditor::setupSyntaxChecker()
{
    m_syntaxChecker = new SyntaxChecker(this);
    connect(m_syntaxChecker, &SyntaxChecker::errorsFound,
            this, &CodeEditor::onSyntaxErrors);
    qDebug() << "[CodeEditor] Syntax checker initialized successfully";
}

void CodeEditor::setupErrorIndicators()
{
    // 错误指示器（红色波浪线）
    m_editor->indicatorDefine(QsciScintilla::SquiggleIndicator, m_errorIndicator);
    m_editor->setIndicatorForegroundColor(QColor(255, 0, 0), m_errorIndicator);
    
    // 警告指示器（黄色波浪线）
    m_editor->indicatorDefine(QsciScintilla::SquiggleIndicator, m_warningIndicator);
    m_editor->setIndicatorForegroundColor(QColor(255, 165, 0), m_warningIndicator);
}

bool CodeEditor::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_editor && event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        if (handleBracketCompletion(keyEvent)) {
            return true;
        }
    }
    return QWidget::eventFilter(obj, event);
}

bool CodeEditor::handleBracketCompletion(QKeyEvent *event)
{
    QString text = event->text();
    
    if (text == "(") {
        insertMatchingBracket("(", ")");
        return true;
    } else if (text == "{") {
        insertMatchingBracket("{", "}");
        return true;
    } else if (text == "[") {
        insertMatchingBracket("[", "]");
        return true;
    } else if (text == "\"") {
        insertMatchingBracket("\"", "\"");
        return true;
    } else if (text == "'") {
        insertMatchingBracket("'", "'");
        return true;
    }
    
    // 智能跳过闭合括号
    if (text == ")" || text == "}" || text == "]" || text == "\"" || text == "'") {
        int line, col;
        m_editor->getCursorPosition(&line, &col);
        QString lineText = m_editor->text(line);
        
        if (col < lineText.length() && lineText.mid(col, 1) == text) {
            // 跳过已存在的闭合符号
            m_editor->setCursorPosition(line, col + 1);
            return true;
        }
    }
    
    return false;
}

void CodeEditor::insertMatchingBracket(const QString &openBracket, const QString &closeBracket)
{
    int line, col;
    m_editor->getCursorPosition(&line, &col);
    
    // 插入开闭括号
    m_editor->insert(openBracket + closeBracket);
    
    // 将光标定位到中间
    m_editor->setCursorPosition(line, col + 1);
}

void CodeEditor::onSyntaxErrors(const QVector<SyntaxError> &errors)
{
    markErrors(errors);
    emit syntaxErrorsFound(errors);
}

void CodeEditor::markErrors(const QVector<SyntaxError> &errors)
{
    clearErrorMarkers();
    
    for (const SyntaxError &error : errors) {
        int indicator = (error.type == "error") ? m_errorIndicator : m_warningIndicator;
        
        // 标记整行
        int lineLength = m_editor->text(error.line - 1).length();
        m_editor->fillIndicatorRange(error.line - 1, 0, error.line - 1, lineLength, indicator);
    }
}

void CodeEditor::clearErrorMarkers()
{
    int totalLines = m_editor->lines();
    m_editor->clearIndicatorRange(0, 0, totalLines, 0, m_errorIndicator);
    m_editor->clearIndicatorRange(0, 0, totalLines, 0, m_warningIndicator);
}

void CodeEditor::checkSyntax()
{
    if (m_syntaxChecker) {
        m_syntaxChecker->checkCode(m_editor->text(), "g++");
    }
}

void CodeEditor::enableSyntaxCheck(bool enabled)
{
    if (enabled && !m_syntaxChecker) {
        setupSyntaxChecker();
    } else if (!enabled && m_syntaxChecker) {
        m_syntaxChecker->deleteLater();
        m_syntaxChecker = nullptr;
        clearErrorMarkers();
    }
}

void CodeEditor::setCompiler(const QString &compiler)
{
    if (m_syntaxChecker) {
        m_syntaxChecker->checkCode(m_editor->text(), compiler);
    }
}

void CodeEditor::setCursorPosition(int line, int col)
{
    m_editor->setCursorPosition(line, col);
}

void CodeEditor::ensureLineVisible(int line)
{
    m_editor->ensureLineVisible(line);
}

void CodeEditor::forceSave()
{
    qDebug() << "[CodeEditor] Force save triggered";
    QString currentCode = m_editor->text();
    m_autoSaver->setContent(currentCode);
    m_autoSaver->forceSave();  // 立即保存
}
