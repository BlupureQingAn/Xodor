#include "CodeEditor.h"
#include "../ai/OllamaClient.h"
#include <QVBoxLayout>
#include <QFont>
#include <QFontInfo>
#include <QKeyEvent>
#include <QDebug>
#include <QRegularExpression>
#include <QTimer>
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
    
    // === 字体设置 - 使用等宽字体 ===
    QFont font;
    #ifdef Q_OS_WIN
        // Windows优先使用Consolas（清晰易读，1和l容易区分）
        QStringList fontFamilies = {"Consolas", "Cascadia Code", "Courier New"};
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
    
    // 禁用自动补全列表的自动排序，保持我们定义的顺序
    m_editor->SendScintilla(QsciScintilla::SCI_AUTOCSETORDER, 0);  // 0 = 保持原始顺序
    
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
    QString code = m_editor->text();
    // 移除末尾的空行，避免行号偏移
    while (code.endsWith("\n") || code.endsWith("\r")) {
        code.chop(1);
    }
    return code;
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
        
        // 处理Tab键补全
        if (keyEvent->key() == Qt::Key_Tab) {
            // 检查是否有自动补全列表显示
            if (m_editor->SendScintilla(QsciScintilla::SCI_AUTOCACTIVE)) {
                // 有补全列表，Tab键选择当前项
                m_editor->SendScintilla(QsciScintilla::SCI_AUTOCCOMPLETE);
                return true;
            }
            // 没有补全列表，执行默认Tab行为（缩进）
            return false;
        }
        
        // 处理回车键：在 {} 之间按回车时自动添加空行
        if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
            int line, col;
            m_editor->getCursorPosition(&line, &col);
            QString lineText = m_editor->text(line);
            
            // 检查光标前后是否是 { 和 }
            if (col > 0 && col < lineText.length()) {
                QChar before = lineText[col - 1];
                QChar after = lineText[col];
                
                if (before == '{' && after == '}') {
                    // 在 {} 之间，插入两个换行和适当的缩进
                    m_editor->beginUndoAction();
                    
                    // 获取当前行的缩进
                    QString indent = "";
                    for (int i = 0; i < lineText.length(); i++) {
                        if (lineText[i] == ' ' || lineText[i] == '\t') {
                            indent += lineText[i];
                        } else {
                            break;
                        }
                    }
                    
                    // 插入换行、缩进、空行、换行、原缩进
                    QString insertion = "\n" + indent + "    \n" + indent;
                    m_editor->insert(insertion);
                    
                    // 将光标移到中间行的末尾
                    m_editor->setCursorPosition(line + 1, indent.length() + 4);
                    
                    m_editor->endUndoAction();
                    return true;
                }
            }
            // 否则执行默认回车行为
            return false;
        }
        
        // 处理Ctrl+Shift组合键
        if ((keyEvent->modifiers() & Qt::ControlModifier) && 
            (keyEvent->modifiers() & Qt::ShiftModifier)) {
            switch (keyEvent->key()) {
                case Qt::Key_Z:  // Ctrl+Shift+Z 重做
                    m_editor->redo();
                    return true;
                case Qt::Key_K:  // Ctrl+Shift+K 删除当前行
                    m_editor->SendScintilla(QsciScintilla::SCI_LINEDELETE);
                    return true;
                case Qt::Key_D:  // Ctrl+Shift+D 复制当前行
                    m_editor->SendScintilla(QsciScintilla::SCI_LINEDUPLICATE);
                    return true;
                case Qt::Key_Up:  // Ctrl+Shift+Up 向上移动行
                    m_editor->SendScintilla(QsciScintilla::SCI_MOVESELECTEDLINESUP);
                    return true;
                case Qt::Key_Down:  // Ctrl+Shift+Down 向下移动行
                    m_editor->SendScintilla(QsciScintilla::SCI_MOVESELECTEDLINESDOWN);
                    return true;
                default:
                    break;
            }
        }
        
        // 处理Ctrl快捷键
        if (keyEvent->modifiers() & Qt::ControlModifier) {
            switch (keyEvent->key()) {
                case Qt::Key_C:  // Ctrl+C 复制
                    m_editor->copy();
                    return true;
                case Qt::Key_X:  // Ctrl+X 剪切
                    m_editor->cut();
                    return true;
                case Qt::Key_V:  // Ctrl+V 粘贴
                    m_editor->paste();
                    return true;
                case Qt::Key_Z:  // Ctrl+Z 撤销
                    m_editor->undo();
                    return true;
                case Qt::Key_Y:  // Ctrl+Y 重做
                    m_editor->redo();
                    return true;
                case Qt::Key_A:  // Ctrl+A 全选
                    m_editor->selectAll();
                    return true;
                case Qt::Key_F:  // Ctrl+F 查找
                    m_editor->findFirst("", false, false, false, true);
                    return true;
                case Qt::Key_H:  // Ctrl+H 替换
                    // QScintilla没有内置替换对话框，可以后续添加
                    return false;
                case Qt::Key_D:  // Ctrl+D 删除当前行
                    m_editor->SendScintilla(QsciScintilla::SCI_LINEDELETE);
                    return true;
                case Qt::Key_Slash:  // Ctrl+/ 注释/取消注释
                    toggleComment();
                    return true;
                case Qt::Key_S:  // Ctrl+S 保存
                    forceSave();
                    return true;
                case Qt::Key_Home:  // Ctrl+Home 跳到文件开头
                    m_editor->SendScintilla(QsciScintilla::SCI_DOCUMENTSTART);
                    return true;
                case Qt::Key_End:  // Ctrl+End 跳到文件结尾
                    m_editor->SendScintilla(QsciScintilla::SCI_DOCUMENTEND);
                    return true;
                case Qt::Key_Left:  // Ctrl+Left 跳到上一个单词
                    m_editor->SendScintilla(QsciScintilla::SCI_WORDLEFT);
                    return true;
                case Qt::Key_Right:  // Ctrl+Right 跳到下一个单词
                    m_editor->SendScintilla(QsciScintilla::SCI_WORDRIGHT);
                    return true;
                default:
                    break;
            }
        }
        
        // 处理括号自动补全
        if (handleBracketCompletion(keyEvent)) {
            return true;
        }
        
        // 处理字母输入时的智能补全（延迟触发）
        // 只在输入字母或下划线时触发，且当前单词至少2个字符
        QString inputText = keyEvent->text();
        if (!inputText.isEmpty() && (inputText[0].isLetter() || inputText[0] == '_')) {
            // 延迟触发，让字符先插入
            QTimer::singleShot(0, this, [this]() {
                // 检查当前单词长度，避免单字符触发
                int line, col;
                m_editor->getCursorPosition(&line, &col);
                QString lineText = m_editor->text(line);
                QString beforeText = lineText.left(col);
                QRegularExpression wordRegex(R"(.*?(\w+)$)");
                QRegularExpressionMatch match = wordRegex.match(beforeText);
                if (match.hasMatch()) {
                    QString word = match.captured(1);
                    if (word.length() >= 2) {  // 至少2个字符才触发
                        handleKeywordCompletion();
                    }
                }
            });
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
    } else if (text == "<") {
        // 智能补全 <>：只在模板类型时补全
        if (shouldCompleteAngleBracket()) {
            insertMatchingBracket("<", ">");
            return true;
        }
    } else if (text == ".") {
        // 让点号先正常插入，然后延迟触发补全
        QTimer::singleShot(0, this, [this]() {
            handleDotCompletion();
        });
        return false;  // 让点号正常插入
    } else if (text == ">") {
        // 检查是否是 -> 箭头操作符
        int line, col;
        m_editor->getCursorPosition(&line, &col);
        QString lineText = m_editor->text(line);
        if (col > 0 && lineText.mid(col - 1, 1) == "-") {
            // 是箭头操作符，延迟触发补全
            QTimer::singleShot(0, this, [this]() {
                handleArrowCompletion();
            });
        }
        return false;  // 让 > 正常插入
    } else if (text == ":") {
        // 检查是否是 :: 作用域操作符
        int line, col;
        m_editor->getCursorPosition(&line, &col);
        QString lineText = m_editor->text(line);
        if (col > 0 && lineText.mid(col - 1, 1) == ":") {
            // 是作用域操作符，延迟触发补全
            QTimer::singleShot(0, this, [this]() {
                handleScopeCompletion();
            });
        }
        return false;  // 让 : 正常插入
    }
    
    // 智能跳过闭合括号（排除 > 因为它用于箭头操作符）
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

bool CodeEditor::shouldCompleteAngleBracket()
{
    int line, col;
    m_editor->getCursorPosition(&line, &col);
    QString lineText = m_editor->text(line);
    
    // 获取 < 之前的文本
    QString beforeText = lineText.left(col).trimmed();
    
    // 如果前面是 >> 或 << 或空格+<，不补全（可能是位移运算符或流操作）
    if (beforeText.endsWith(">>") || beforeText.endsWith("<<") || 
        beforeText.endsWith(" ") || beforeText.isEmpty()) {
        return false;
    }
    
    // 常见的 C++ 模板类型关键词
    QStringList templateKeywords = {
        "vector", "map", "set", "list", "deque", "queue", "stack",
        "unordered_map", "unordered_set", "priority_queue",
        "pair", "tuple", "array", "shared_ptr", "unique_ptr", "weak_ptr",
        "optional", "variant", "function", "basic_string"
    };
    
    // 检查是否以模板关键词结尾
    for (const QString &keyword : templateKeywords) {
        if (beforeText.endsWith(keyword)) {
            return true;
        }
    }
    
    // 如果前面是标识符（字母、数字、下划线）且不是单个字符，可能是自定义模板类
    if (col > 0 && beforeText.length() > 1) {
        QChar lastChar = beforeText[beforeText.length() - 1];
        if (lastChar.isLetterOrNumber() || lastChar == '_') {
            // 检查是否是类型名（首字母大写或全大写）
            QStringList words = beforeText.split(QRegularExpression("\\W+"));
            if (!words.isEmpty()) {
                QString lastWord = words.last();
                if (!lastWord.isEmpty() && (lastWord[0].isUpper() || lastWord == lastWord.toUpper())) {
                    return true;
                }
            }
        }
    }
    
    return false;
}

bool CodeEditor::handleDotCompletion()
{
    qDebug() << "[handleDotCompletion] ===== CALLED =====";
    
    int line, col;
    m_editor->getCursorPosition(&line, &col);
    QString lineText = m_editor->text(line);
    QString allText = m_editor->text();
    
    qDebug() << "[handleDotCompletion] lineText:" << lineText;
    qDebug() << "[handleDotCompletion] col:" << col;
    
    // 获取点号之前的文本（不trim，保留原始格式）
    // 注意：由于延迟触发，此时点号已经插入，需要去掉最后的点号
    QString beforeText = lineText.left(col);
    if (beforeText.endsWith('.')) {
        beforeText.chop(1);  // 去掉最后的点号
    }
    qDebug() << "[handleDotCompletion] beforeText:" << beforeText;
    
    // 提取最后一个标识符或表达式（包括可能的数组访问）
    // 匹配：标识符 或 标识符[索引]，可能前面有其他字符
    // .*? 非贪婪匹配任意前缀（如空格、cin>>等）
    QRegularExpression lastTokenRegex(R"(.*?(\w+)(\s*\[\s*[^\]]+\s*\])?\s*$)");
    QRegularExpressionMatch match = lastTokenRegex.match(beforeText);
    
    if (!match.hasMatch()) {
        qDebug() << "[handleDotCompletion] No match for beforeText:" << beforeText;
        return false;
    }
    
    QString varName = match.captured(1);  // 第一个捕获组是变量名
    bool isArrayAccess = !match.captured(2).isEmpty();  // 第二个捕获组是 [...]
    
    qDebug() << "[handleDotCompletion] varName:" << varName << "isArrayAccess:" << isArrayAccess;
    QStringList completions;
    
    // 1. 检查 pair 类型
    // 直接声明为 pair
    QString pairPattern = QString(R"(\bpair\s*<[^>]+>\s+%1\b)").arg(varName);
    qDebug() << "[handleDotCompletion] Checking pair pattern:" << pairPattern;
    if (allText.contains(QRegularExpression(pairPattern))) {
        qDebug() << "[handleDotCompletion] Matched direct pair!";
        completions << "first" << "second";
    }
    // 或者是容器<pair> 的元素访问
    else if (isArrayAccess) {
        // 检查 vector<pair>, deque<pair>, array<pair> 等
        QString containerPairPattern = QString(R"(\b(?:vector|deque|array)\s*<\s*pair\s*<[^>]+>\s*(?:,\s*\d+)?\s*>\s+%1\b)").arg(varName);
        qDebug() << "[handleDotCompletion] Checking container<pair> pattern:" << containerPairPattern;
        if (allText.contains(QRegularExpression(containerPairPattern))) {
            qDebug() << "[handleDotCompletion] Matched container<pair>!";
            completions << "first" << "second";
        }
        // 2. 检查 vector<string> 的元素访问
        else if (allText.contains(QRegularExpression(QString(R"(\bvector\s*<\s*string\s*>\s+%1\b)").arg(varName)))) {
            completions << "size()" << "length()" << "empty()" << "clear()" 
                       << "substr()" << "find()" << "c_str()" << "push_back()" << "pop_back()"
                       << "append()" << "insert()" << "erase()" << "replace()"
                       << "begin()" << "end()" << "front()" << "back()";
        }
        // 3. 检查 deque<string> 的元素访问
        else if (allText.contains(QRegularExpression(QString(R"(\bdeque\s*<\s*string\s*>\s+%1\b)").arg(varName)))) {
            completions << "size()" << "length()" << "empty()" << "clear()" 
                       << "substr()" << "find()" << "c_str()" << "push_back()" << "pop_back()"
                       << "append()" << "insert()" << "erase()" << "replace()"
                       << "begin()" << "end()" << "front()" << "back()";
        }
        // 4. 检查 vector<vector> 嵌套容器的元素访问
        else if (allText.contains(QRegularExpression(QString(R"(\bvector\s*<\s*vector\s*<[^>]+>\s*>\s+%1\b)").arg(varName)))) {
            // v[i] 是一个 vector，提供 vector 的成员
            completions << "size()" << "empty()" << "push_back()" << "pop_back()" 
                       << "clear()" << "front()" << "back()" << "begin()" << "end()"
                       << "insert()" << "erase()" << "resize()" << "reserve()";
        }
        // 5. 检查 list<string> 的元素访问
        else if (allText.contains(QRegularExpression(QString(R"(\blist\s*<\s*string\s*>\s+%1\b)").arg(varName)))) {
            completions << "size()" << "length()" << "empty()" << "clear()" 
                       << "substr()" << "find()" << "c_str()" << "push_back()" << "pop_back()"
                       << "append()" << "insert()" << "erase()" << "replace()"
                       << "begin()" << "end()" << "front()" << "back()";
        }
        // 6. 检查 array<string> 的元素访问
        else if (allText.contains(QRegularExpression(QString(R"(\barray\s*<\s*string\s*,\s*\d+\s*>\s+%1\b)").arg(varName)))) {
            completions << "size()" << "length()" << "empty()" << "clear()" 
                       << "substr()" << "find()" << "c_str()" << "push_back()" << "pop_back()"
                       << "append()" << "insert()" << "erase()" << "replace()"
                       << "begin()" << "end()" << "front()" << "back()";
        }
        // 如果是其他类型的数组访问（如 vector<int>[i]），不提供补全
    }
    
    // 3. 检查 vector 本身的成员（非数组访问）
    else if (!isArrayAccess && allText.contains(QRegularExpression(QString(R"(\bvector\s*<[^>]+>\s+%1\b)").arg(varName)))) {
        // 访问 vector 本身的成员
        completions << "size()" << "empty()" << "push_back()" << "pop_back()" 
                   << "clear()" << "front()" << "back()" << "begin()" << "end()"
                   << "insert()" << "erase()" << "resize()" << "reserve()";
    }
    
    // 4. 检查 string 类型
    else if (allText.contains(QRegularExpression(QString(R"(\bstring\s+%1\b)").arg(varName)))) {
        completions << "size()" << "length()" << "empty()" << "clear()" 
                   << "substr()" << "find()" << "c_str()" << "push_back()" << "pop_back()"
                   << "append()" << "insert()" << "erase()" << "replace()"
                   << "begin()" << "end()" << "front()" << "back()";
    }
    
    // 5. 检查 map/unordered_map 类型
    else if (allText.contains(QRegularExpression(QString(R"(\b(?:unordered_)?map\s*<[^>]+>\s+%1\b)").arg(varName)))) {
        if (!isArrayAccess) {
            completions << "size()" << "empty()" << "clear()" << "insert()" << "erase()"
                       << "find()" << "count()" << "begin()" << "end()";
        } else {
            // map[key] 访问返回值类型
            // 尝试提取 value 类型并提供相应补全
            QRegularExpression mapTypeRegex(QString(R"(\b(?:unordered_)?map\s*<\s*[^,]+\s*,\s*(\w+)\s*>\s+%1\b)").arg(varName));
            QRegularExpressionMatch mapMatch = mapTypeRegex.match(allText);
            if (mapMatch.hasMatch()) {
                QString valueType = mapMatch.captured(1);
                if (valueType == "string") {
                    completions << "size()" << "length()" << "empty()" << "clear()" 
                               << "substr()" << "find()" << "c_str()" << "push_back()" << "pop_back()"
                               << "append()" << "insert()" << "erase()" << "replace()"
                               << "begin()" << "end()" << "front()" << "back()";
                }
                // 其他基本类型不提供补全
            }
        }
    }
    
    // 6. 检查 set/unordered_set 类型
    else if (allText.contains(QRegularExpression(QString(R"(\b(?:unordered_)?set\s*<[^>]+>\s+%1\b)").arg(varName)))) {
        if (!isArrayAccess) {
            completions << "size()" << "empty()" << "clear()" << "insert()" << "erase()"
                       << "find()" << "count()" << "begin()" << "end()";
        }
    }
    
    // 7. 检查 queue 类型
    else if (allText.contains(QRegularExpression(QString(R"(\bqueue\s*<[^>]+>\s+%1\b)").arg(varName)))) {
        if (!isArrayAccess) {
            completions << "size()" << "empty()" << "push()" << "pop()" << "front()" << "back()";
        }
    }
    
    // 8. 检查 stack 类型
    else if (allText.contains(QRegularExpression(QString(R"(\bstack\s*<[^>]+>\s+%1\b)").arg(varName)))) {
        if (!isArrayAccess) {
            completions << "size()" << "empty()" << "push()" << "pop()" << "top()";
        }
    }
    
    // 9. 检查 priority_queue 类型
    else if (allText.contains(QRegularExpression(QString(R"(\bpriority_queue\s*<[^>]+>\s+%1\b)").arg(varName)))) {
        if (!isArrayAccess) {
            completions << "size()" << "empty()" << "push()" << "pop()" << "top()";
        }
    }
    
    // 10. 检查 deque 类型
    else if (allText.contains(QRegularExpression(QString(R"(\bdeque\s*<[^>]+>\s+%1\b)").arg(varName)))) {
        if (!isArrayAccess) {
            completions << "size()" << "empty()" << "push_back()" << "pop_back()" 
                       << "push_front()" << "pop_front()" << "clear()" 
                       << "front()" << "back()" << "begin()" << "end()";
        }
    }
    
    // 11. 检查 list 类型
    else if (allText.contains(QRegularExpression(QString(R"(\blist\s*<[^>]+>\s+%1\b)").arg(varName)))) {
        if (!isArrayAccess) {
            completions << "size()" << "empty()" << "push_back()" << "pop_back()" 
                       << "push_front()" << "pop_front()" << "clear()"
                       << "front()" << "back()" << "begin()" << "end()" 
                       << "sort()" << "reverse()";
        }
    }
    
    // 12. 检查 array 类型
    else if (allText.contains(QRegularExpression(QString(R"(\barray\s*<[^>]+>\s+%1\b)").arg(varName)))) {
        if (!isArrayAccess) {
            completions << "size()" << "empty()" << "front()" << "back()" 
                       << "begin()" << "end()" << "fill()";
        }
    }
    
    // 13. 检查 tuple 类型（虽然不常用 . 访问，但提供 get 提示）
    else if (allText.contains(QRegularExpression(QString(R"(\btuple\s*<[^>]+>\s+%1\b)").arg(varName)))) {
        completions << "swap()";  // tuple 主要用 std::get<>() 访问
    }
    
    // 14. 检查智能指针类型
    else if (allText.contains(QRegularExpression(QString(R"(\b(?:shared_ptr|unique_ptr|weak_ptr)\s*<[^>]+>\s+%1\b)").arg(varName)))) {
        completions << "get()" << "reset()" << "swap()";
        if (allText.contains("shared_ptr")) {
            completions << "use_count()";
        }
        if (allText.contains("weak_ptr")) {
            completions << "lock()" << "expired()";
        }
    }
    
    // 15. 检查 typedef/using 别名（常见的）
    // 例如：typedef pair<int,int> pii; pii p; p.
    else if (allText.contains(QRegularExpression(QString(R"(\btypedef\s+pair\s*<[^>]+>\s+\w+\s*;.*\b\w+\s+%1\b)").arg(varName)))) {
        completions << "first" << "second";
    }
    
    // 16. 检查 auto 类型（尝试推断）
    else if (allText.contains(QRegularExpression(QString(R"(\bauto\s+%1\s*=)").arg(varName)))) {
        // 尝试从赋值语句推断类型
        QRegularExpression autoAssignRegex(QString(R"(\bauto\s+%1\s*=\s*([^;]+))").arg(varName));
        QRegularExpressionMatch autoMatch = autoAssignRegex.match(allText);
        if (autoMatch.hasMatch()) {
            QString assignValue = autoMatch.captured(1).trimmed();
            
            // 检查是否是 make_pair
            if (assignValue.contains("make_pair")) {
                completions << "first" << "second";
            }
            // 检查是否是容器的 begin/end
            else if (assignValue.contains(".begin()") || assignValue.contains(".end()")) {
                // 可能是迭代器，提供通用建议
                if (!isArrayAccess) {
                    completions << "first" << "second";  // 假设是 map 迭代器
                }
            }
        }
    }
    
    // 17. 兜底检查：常见的迭代器命名（仅当前面没有匹配时）
    // 例如：auto it = find(...); it.
    if (completions.isEmpty() && !isArrayAccess) {
        if (varName.contains("it") || varName == "iter" || varName == "iterator") {
            // 可能是迭代器，但不确定类型，提供通用建议
            completions << "first" << "second";  // 对于 map 迭代器
        }
    }
    
    // 如果找到了补全项且数量合理
    if (!completions.isEmpty() && completions.size() <= 30) {
        // 使用QScintilla的自动补全API
        // 设置分隔符为空格
        m_editor->SendScintilla(QsciScintilla::SCI_AUTOCSETSEPARATOR, ' ');
        // 显示补全列表（参数：最小匹配长度，补全列表）
        m_editor->SendScintilla(QsciScintilla::SCI_AUTOCSHOW, (uintptr_t)0, completions.join(" ").toUtf8().constData());
        
        return true;
    }
    
    return false;
}

bool CodeEditor::handleArrowCompletion()
{
    int line, col;
    m_editor->getCursorPosition(&line, &col);
    QString lineText = m_editor->text(line);
    QString allText = m_editor->text();
    
    // 获取 -> 之前的文本（不trim）
    QString beforeText = lineText.left(col);  // col 已经包含了 ->
    
    // 提取最后一个标识符（可能包括数组访问）
    // .*? 非贪婪匹配任意前缀
    QRegularExpression lastTokenRegex(R"(.*?(\w+)(\s*\[\s*[^\]]+\s*\])?\s*->$)");
    QRegularExpressionMatch match = lastTokenRegex.match(beforeText);
    
    if (!match.hasMatch()) {
        qDebug() << "[handleArrowCompletion] No match for beforeText:" << beforeText;
        return false;
    }
    
    QString varName = match.captured(1);
    qDebug() << "[handleArrowCompletion] varName:" << varName;
    QStringList completions;
    
    // 1. 检查智能指针类型
    if (allText.contains(QRegularExpression(QString(R"(\b(?:shared_ptr|unique_ptr)\s*<\s*string\s*>\s+%1\b)").arg(varName)))) {
        completions << "size()" << "length()" << "empty()" << "clear()" 
                   << "substr()" << "find()" << "c_str()" << "push_back()" << "pop_back()"
                   << "append()" << "insert()" << "erase()" << "replace()"
                   << "begin()" << "end()" << "front()" << "back()";
    }
    else if (allText.contains(QRegularExpression(QString(R"(\b(?:shared_ptr|unique_ptr)\s*<\s*vector\s*<[^>]+>\s*>\s+%1\b)").arg(varName)))) {
        completions << "size()" << "empty()" << "push_back()" << "pop_back()" 
                   << "clear()" << "front()" << "back()" << "begin()" << "end()"
                   << "insert()" << "erase()" << "resize()" << "reserve()";
    }
    
    // 2. 检查是否是 map/set 的迭代器
    else if (varName.contains("it") || varName.contains("iter")) {
        // 检查是否是 map 的迭代器
        if (allText.contains(QRegularExpression(R"(\b(?:unordered_)?map\s*<[^>]+>)"))) {
            completions << "first" << "second";  // map 迭代器指向 pair
        }
        // 其他迭代器可能指向普通元素，不提供补全
    }
    
    // 3. 检查是否是指针类型的变量
    else if (allText.contains(QRegularExpression(QString(R"(\bstring\s*\*\s*%1)").arg(varName)))) {
        completions << "size()" << "length()" << "empty()" << "clear()" 
                   << "substr()" << "find()" << "c_str()" << "push_back()" << "pop_back()"
                   << "append()" << "insert()" << "erase()" << "replace()"
                   << "begin()" << "end()" << "front()" << "back()";
    }
    else if (allText.contains(QRegularExpression(QString(R"(\bvector\s*<[^>]+>\s*\*\s*%1)").arg(varName)))) {
        completions << "size()" << "empty()" << "push_back()" << "pop_back()" 
                   << "clear()" << "front()" << "back()" << "begin()" << "end()"
                   << "insert()" << "erase()" << "resize()" << "reserve()";
    }
    
    if (!completions.isEmpty() && completions.size() <= 30) {
        // 使用QScintilla的自动补全API
        m_editor->SendScintilla(QsciScintilla::SCI_AUTOCSETSEPARATOR, ' ');
        m_editor->SendScintilla(QsciScintilla::SCI_AUTOCSHOW, (uintptr_t)0, completions.join(" ").toUtf8().constData());
        
        return true;
    }
    
    return false;
}

bool CodeEditor::handleScopeCompletion()
{
    int line, col;
    m_editor->getCursorPosition(&line, &col);
    QString lineText = m_editor->text(line);
    
    // 获取 :: 之前的文本（不trim）
    QString beforeText = lineText.left(col);  // col 已经包含了 ::
    
    // 提取最后一个标识符
    // .*? 非贪婪匹配任意前缀
    QRegularExpression lastTokenRegex(R"(.*?(\w+)\s*::$)");
    QRegularExpressionMatch match = lastTokenRegex.match(beforeText);
    
    if (!match.hasMatch()) {
        qDebug() << "[handleScopeCompletion] No match for beforeText:" << beforeText;
        return false;
    }
    
    QString namespaceName = match.captured(1);
    qDebug() << "[handleScopeCompletion] namespaceName:" << namespaceName;
    QStringList completions;
    
    // std 命名空间
    if (namespaceName == "std") {
        completions << "vector" << "map" << "set" << "unordered_map" << "unordered_set"
                   << "string" << "pair" << "queue" << "stack" << "priority_queue"
                   << "deque" << "list" << "array" << "tuple"
                   << "cout" << "cin" << "endl" << "sort" << "max" << "min"
                   << "swap" << "reverse" << "find" << "count" << "accumulate"
                   << "make_pair" << "make_tuple";
    }
    
    if (!completions.isEmpty() && completions.size() <= 30) {
        // 使用QScintilla的自动补全API
        m_editor->SendScintilla(QsciScintilla::SCI_AUTOCSETSEPARATOR, ' ');
        m_editor->SendScintilla(QsciScintilla::SCI_AUTOCSHOW, (uintptr_t)0, completions.join(" ").toUtf8().constData());
        
        return true;
    }
    
    return false;
}

void CodeEditor::handleKeywordCompletion()
{
    int line, col;
    m_editor->getCursorPosition(&line, &col);
    QString lineText = m_editor->text(line);
    
    // 获取当前光标前的文本
    QString beforeText = lineText.left(col);
    
    // 提取最后输入的单词（至少2个字符）
    // .*? 非贪婪匹配任意前缀（如空格、运算符等）
    QRegularExpression wordRegex(R"(.*?(\w{2,})$)");
    QRegularExpressionMatch match = wordRegex.match(beforeText);
    
    if (!match.hasMatch()) {
        return;
    }
    
    QString currentWord = match.captured(1);
    QStringList completions;
    
    // 1. 提取当前代码中的变量名
    QString allText = m_editor->text();
    QStringList variableNames;
    
    // 提取基本类型变量：int n, int n,m, double x
    // 匹配类型后的所有变量名（包括逗号分隔的）
    QRegularExpression basicVarRegex(R"(\b(?:int|long|short|char|bool|float|double|void|size_t|auto)\s+([\w\s,]+)[;=)])");
    QRegularExpressionMatchIterator it = basicVarRegex.globalMatch(allText);
    while (it.hasNext()) {
        QRegularExpressionMatch m = it.next();
        QString varList = m.captured(1);
        // 分割逗号分隔的变量名
        QStringList vars = varList.split(QRegularExpression("[,\\s]+"), Qt::SkipEmptyParts);
        for (const QString &varName : vars) {
            if (!variableNames.contains(varName) && varName.length() > 0) {
                variableNames << varName;
            }
        }
    }
    
    // 提取 STL 容器变量：vector<int> v, vector<pair<int,int>> d(n)
    // 使用更宽松的匹配：容器名后跟 < ... > 再跟变量名
    // 注意：这里使用非贪婪匹配 .+? 来匹配尖括号内的内容（包括嵌套的<>）
    QRegularExpression containerVarRegex(R"(\b(?:vector|map|set|string|pair|queue|stack|deque|list|array|unordered_map|unordered_set|priority_queue|bitset|forward_list|multimap|multiset)\s*<.+?>\s+(\w+))");
    it = containerVarRegex.globalMatch(allText);
    while (it.hasNext()) {
        QRegularExpressionMatch m = it.next();
        QString varName = m.captured(1);
        if (!variableNames.contains(varName)) {
            variableNames << varName;
        }
    }
    
    // 提取循环变量：for(int i=0; ...)
    QRegularExpression forVarRegex(R"(\bfor\s*\(\s*(?:int|long|short|size_t|auto)\s+(\w+))");
    it = forVarRegex.globalMatch(allText);
    while (it.hasNext()) {
        QRegularExpressionMatch m = it.next();
        QString varName = m.captured(1);
        if (!variableNames.contains(varName)) {
            variableNames << varName;
        }
    }
    
    // 提取范围for变量：for(auto x : v)
    QRegularExpression rangeForRegex(R"(\bfor\s*\(\s*(?:auto|const\s+auto)\s*&?\s*(\w+)\s*:)");
    it = rangeForRegex.globalMatch(allText);
    while (it.hasNext()) {
        QRegularExpressionMatch m = it.next();
        QString varName = m.captured(1);
        if (!variableNames.contains(varName)) {
            variableNames << varName;
        }
    }
    
    // 添加匹配的变量名到补全列表
    for (const QString &varName : variableNames) {
        if (varName.startsWith(currentWord, Qt::CaseInsensitive) && varName != currentWord) {
            completions << varName;
        }
    }
    
    // 2. 构建完整的关键字和函数列表
    static QStringList keywords = {
        // C++ 基本关键字
        "if", "else", "for", "while", "do", "switch", "case", "default",
        "break", "continue", "return",
        "const", "static", "extern", "inline", "virtual",
        "class", "struct", "enum", "union", "namespace", "typedef", "using",
        "public", "private", "protected",
        "template", "typename", "auto",
        "void", "int", "char", "bool", "double", "float", "long", "short",
        "unsigned", "signed", "size_t",
        
        // STL 容器
        "vector", "map", "set", "unordered_map", "unordered_set",
        "multimap", "multiset", "unordered_multimap", "unordered_multiset",
        "string", "pair", "tuple",
        "queue", "stack", "priority_queue", "deque", "list",
        "array", "bitset", "forward_list",
        
        // STL 算法和函数
        "sort", "stable_sort", "partial_sort",
        "find", "find_if", "find_if_not",
        "binary_search", "lower_bound", "upper_bound", "equal_range",
        "min", "max", "min_element", "max_element",
        "swap", "reverse", "rotate",
        "fill", "fill_n",
        
        // iostream (常用的放前面，cout 必须在 count 之前)
        "cout", "cin", "endl", "cerr",
        
        // 计数函数放在 cout 后面
        "count_if", "count",
        "accumulate", "reduce",
        "unique", "remove", "remove_if",
        "next_permutation", "prev_permutation",
        "make_pair", "make_tuple",
        
        // 常用成员函数（提示用）
        "push_back", "pop_back", "emplace_back",
        "push_front", "pop_front", "emplace_front",
        "insert", "erase", "clear",
        "begin", "end", "rbegin", "rend",
        "front", "back",
        "size", "empty", "resize", "reserve", "capacity",
        
        // 其他常用
        "std", "nullptr", "true", "false",
        "sizeof", "new", "delete",
        "try", "catch", "throw"
    };
    
    // 过滤出匹配的补全项（在添加时就过滤）
    for (const QString &keyword : keywords) {
        if (keyword.startsWith(currentWord, Qt::CaseInsensitive) && keyword != currentWord) {
            completions << keyword;
        }
    }
    
    // 去重（虽然 keywords 列表应该已经没有重复）
    completions.removeDuplicates();
    
    // 如果有补全项且数量合理，显示补全列表
    if (!completions.isEmpty() && completions.size() <= 30) {
        m_editor->SendScintilla(QsciScintilla::SCI_AUTOCSETSEPARATOR, ' ');
        m_editor->SendScintilla(QsciScintilla::SCI_AUTOCSHOW, (uintptr_t)currentWord.length(), 
                               completions.join(" ").toUtf8().constData());
    }
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
    // 获取当前光标所在行
    int currentLine, currentCol;
    m_editor->getCursorPosition(&currentLine, &currentCol);
    currentLine++;  // 转换为1-based行号（与错误报告一致）
    
    // 过滤掉当前正在编辑的行的错误
    QVector<SyntaxError> filteredErrors;
    for (const SyntaxError &error : errors) {
        if (error.line != currentLine) {
            filteredErrors.append(error);
        }
    }
    
    markErrors(filteredErrors);
    emit syntaxErrorsFound(filteredErrors);
}

void CodeEditor::markErrors(const QVector<SyntaxError> &errors)
{
    clearErrorMarkers();
    
    for (const SyntaxError &error : errors) {
        int indicator = (error.type == "error") ? m_errorIndicator : m_warningIndicator;
        
        int line = error.line - 1;  // QScintilla行号从0开始
        int col = error.column - 1;  // QScintilla列号从0开始
        
        // 获取该行文本
        QString lineText = m_editor->text(line);
        if (lineText.isEmpty() || col < 0 || col >= lineText.length()) {
            // 如果列号无效，标记整行
            int lineLength = lineText.length();
            m_editor->fillIndicatorRange(line, 0, line, lineLength, indicator);
            continue;
        }
        
        // 精准标记：从错误位置开始，找到单词或符号的结束位置
        int startCol = col;
        int endCol = col;
        
        // 向后查找单词边界
        while (endCol < lineText.length()) {
            QChar ch = lineText[endCol];
            // 如果是字母、数字、下划线，继续
            if (ch.isLetterOrNumber() || ch == '_') {
                endCol++;
            } else {
                break;
            }
        }
        
        // 如果没有找到单词，至少标记一个字符
        if (endCol == startCol) {
            endCol = qMin(startCol + 1, lineText.length());
        }
        
        // 标记错误范围（精准到单词）
        m_editor->fillIndicatorRange(line, startCol, line, endCol, indicator);
        
        qDebug() << "[CodeEditor] Marked error at line" << (line+1) 
                 << "col" << startCol << "-" << endCol 
                 << ":" << error.message;
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

void CodeEditor::toggleComment()
{
    // 获取选中的行范围
    int lineFrom, indexFrom, lineTo, indexTo;
    m_editor->getSelection(&lineFrom, &indexFrom, &lineTo, &indexTo);
    
    // 如果没有选中，处理当前行
    if (lineFrom == -1) {
        m_editor->getCursorPosition(&lineFrom, &indexFrom);
        lineTo = lineFrom;
    }
    
    // 检查是否所有行都已注释
    bool allCommented = true;
    for (int line = lineFrom; line <= lineTo; ++line) {
        QString lineText = m_editor->text(line).trimmed();
        if (!lineText.isEmpty() && !lineText.startsWith("//")) {
            allCommented = false;
            break;
        }
    }
    
    // 开始编辑操作（支持撤销）
    m_editor->beginUndoAction();
    
    if (allCommented) {
        // 取消注释
        for (int line = lineFrom; line <= lineTo; ++line) {
            QString lineText = m_editor->text(line);
            int commentPos = lineText.indexOf("//");
            if (commentPos >= 0) {
                m_editor->setSelection(line, commentPos, line, commentPos + 2);
                m_editor->removeSelectedText();
            }
        }
    } else {
        // 添加注释
        for (int line = lineFrom; line <= lineTo; ++line) {
            QString lineText = m_editor->text(line);
            // 找到第一个非空白字符的位置
            int firstNonSpace = 0;
            while (firstNonSpace < lineText.length() && lineText[firstNonSpace].isSpace()) {
                firstNonSpace++;
            }
            // 在第一个非空白字符前插入注释
            m_editor->insertAt("// ", line, firstNonSpace);
        }
    }
    
    m_editor->endUndoAction();
}
