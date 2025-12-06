#include "ChatBubbleWidget.h"
#include <QPainter>
#include <QPainterPath>
#include <QRegularExpression>
#include <QScrollArea>
#include <QScrollBar>
#include <QWheelEvent>
#include <QApplication>
#include <QAbstractTextDocumentLayout>
#include <QTextCursor>
#include <QTextBlock>
#include <QTextLayout>
#include <QTimer>

ChatBubbleWidget::ChatBubbleWidget(const QString &content, bool isUser, QWidget *parent)
    : QWidget(parent)
    , m_content(content)
    , m_isUser(isUser)
    , m_fontScale(1.0)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    
    QVBoxLayout *layout = new QVBoxLayout(this);
    // 设置布局边距：左右保持，上下增加到10px
    layout->setContentsMargins(isUser ? 10 : 5, 10, isUser ? 5 : 10, 10);
    
    m_textBrowser = new QTextBrowser(this);
    m_textBrowser->setOpenExternalLinks(false);
    m_textBrowser->setReadOnly(true);
    m_textBrowser->setFrameShape(QFrame::NoFrame);
    m_textBrowser->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_textBrowser->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    // 关键：让 QTextBrowser 根据 widget 宽度自动换行
    m_textBrowser->setLineWrapMode(QTextEdit::WidgetWidth);
    m_textBrowser->setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    m_textBrowser->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    m_textBrowser->viewport()->installEventFilter(this);
    
    // 设置文档边距为0，避免额外空白
    m_textBrowser->document()->setDocumentMargin(0);
    m_textBrowser->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
    m_textBrowser->setCursor(Qt::IBeamCursor);
    
    m_textBrowser->setStyleSheet(R"(
        QTextBrowser {
            background: transparent;
            border: none;
            color: #f0f0f0;
            selection-background-color: #4a90e2;
            selection-color: #ffffff;
        }
        QScrollBar:vertical {
            background: transparent;
            width: 8px;
            margin: 2px;
        }
        QScrollBar::handle:vertical {
            background: #4a4a4a;
            border-radius: 4px;
            min-height: 20px;
        }
        QScrollBar::handle:vertical:hover {
            background: #5a5a5a;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0px;
        }
        QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {
            background: none;
        }
    )");
    
    layout->addWidget(m_textBrowser);
    
    // 设置滚动条的滚动步幅（减小步幅使滚动更平滑）
    if (m_textBrowser->verticalScrollBar()) {
        m_textBrowser->verticalScrollBar()->setSingleStep(10);  // 默认是15，改为10
    }
    
    // 设置内容
    setContent(content);
}

void ChatBubbleWidget::setContent(const QString &content)
{
    m_content = content;
    
    if (m_isUser) {
        // 用户消息：使用纯文本模式，避免HTML渲染问题
        m_textBrowser->setPlainText(content.trimmed());
        
        // 设置字体
        int fontSize = qRound(11 * m_fontScale);
        QFont font = m_textBrowser->font();
        font.setPointSize(fontSize);
        m_textBrowser->setFont(font);
        
        // 关键：设置固定行高，避免不同文本内容导致高度差异
        QTextDocument *doc = m_textBrowser->document();
        QTextOption textOption = doc->defaultTextOption();
        textOption.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
        doc->setDefaultTextOption(textOption);
        
        // 设置固定行高（使用QTextBlockFormat）
        QTextCursor cursor(doc);
        cursor.select(QTextCursor::Document);
        QTextBlockFormat blockFormat = cursor.blockFormat();
        QFontMetrics fm(font);
        blockFormat.setLineHeight(fm.lineSpacing(), QTextBlockFormat::FixedHeight);
        cursor.setBlockFormat(blockFormat);
        
        // 设置文本颜色
        QPalette palette = m_textBrowser->palette();
        palette.setColor(QPalette::Text, QColor(240, 240, 240));
        m_textBrowser->setPalette(palette);
    } else {
        // AI消息：使用HTML格式支持Markdown
        QString html = formatMarkdown(content);
        m_textBrowser->setHtml(html);
        
        // 移除QTextDocument末尾可能的空段落
        QTextDocument *doc = m_textBrowser->document();
        QTextCursor cursor(doc);
        cursor.movePosition(QTextCursor::End);
        
        // 如果末尾有空块，删除它
        QTextBlock lastBlock = doc->lastBlock();
        if (lastBlock.isValid() && lastBlock.text().trimmed().isEmpty() && doc->blockCount() > 1) {
            cursor.movePosition(QTextCursor::End);
            cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::KeepAnchor);
            cursor.removeSelectedText();
            cursor.deletePreviousChar(); // 删除换行符
        }
    }
    
    // 让 Qt 自动计算高度
    adjustHeight();
}

void ChatBubbleWidget::adjustHeight()
{
    QTextDocument *doc = m_textBrowser->document();
    
    // 获取可用宽度：使用widget的实际宽度减去布局边距
    int availableWidth = width();
    if (availableWidth <= 0) {
        // 如果宽度还没确定，延迟到下次
        QTimer::singleShot(0, this, &ChatBubbleWidget::adjustHeight);
        return;
    }
    
    // 减去布局边距（左右边距）
    int leftMargin = m_isUser ? 10 : 5;
    int rightMargin = m_isUser ? 5 : 10;
    int textWidth = availableWidth - leftMargin - rightMargin;
    
    // 设置文档宽度
    doc->setTextWidth(textWidth);
    
    if (m_isUser) {
        // 用户消息：使用文档的自然高度
        QAbstractTextDocumentLayout *layout = doc->documentLayout();
        QSizeF docSize = layout->documentSize();
        
        int contentHeight = qCeil(docSize.height());
        int textBrowserHeight = contentHeight + 8;  // 添加小的上下边距
        
        m_textBrowser->setFixedHeight(textBrowserHeight);
        
        // Widget 的高度 = QTextBrowser 高度 + 布局边距（上下各10px）
        int widgetHeight = textBrowserHeight + 20;
        setMinimumHeight(widgetHeight);
        setMaximumHeight(widgetHeight);
    } else {
        // AI消息使用原来的方法（因为有Markdown格式）
        doc->setTextWidth(m_textBrowser->viewport()->width());
        
        QAbstractTextDocumentLayout *layout = doc->documentLayout();
        QSizeF docSize = layout->documentSize();
        
        int contentHeight = qCeil(docSize.height());
        int textBrowserHeight = contentHeight + 8;
        
        // 设置最大高度限制为600px，超过则显示滚动条
        const int maxBubbleHeight = 600;
        bool needsScrollBar = textBrowserHeight > maxBubbleHeight;
        
        if (needsScrollBar) {
            textBrowserHeight = maxBubbleHeight;
            
            int scrollBarWidth = 10;
            doc->setTextWidth(m_textBrowser->viewport()->width() - scrollBarWidth);
            
            docSize = layout->documentSize();
            contentHeight = qCeil(docSize.height());
            int newTextBrowserHeight = contentHeight + 8;
            
            if (newTextBrowserHeight > maxBubbleHeight) {
                textBrowserHeight = maxBubbleHeight;
            } else {
                textBrowserHeight = newTextBrowserHeight;
            }
        }
        
        m_textBrowser->setFixedHeight(textBrowserHeight);
        
        // Widget 的高度 = QTextBrowser 高度 + 布局边距（上下各10px）
        int widgetHeight = textBrowserHeight + 20;
        setMinimumHeight(widgetHeight);
        setMaximumHeight(widgetHeight);
    }
}

QString ChatBubbleWidget::content() const
{
    return m_content;
}

void ChatBubbleWidget::setFontScale(qreal scale)
{
    if (scale < 0.5) scale = 0.5;
    if (scale > 2.0) scale = 2.0;
    
    if (qAbs(m_fontScale - scale) > 0.01) {
        m_fontScale = scale;
        setContent(m_content);  // 重新设置内容以应用新字体大小
    }
}

QString ChatBubbleWidget::formatUserMessage(const QString &content)
{
    // 此函数已不再使用，用户消息现在使用纯文本模式
    // 保留此函数以防需要回退
    int fontSize = qRound(11 * m_fontScale);
    QString trimmed = content.trimmed();
    
    QString escaped = trimmed;
    escaped.replace("&", "&amp;");
    escaped.replace("<", "&lt;");
    escaped.replace(">", "&gt;");
    escaped.replace("\n", "<br>");
    
    while (escaped.endsWith("<br>")) {
        escaped.chop(4);
    }
    
    return QString("<span style='color: #f0f0f0; font-size: %1pt; line-height: 1.5;'>%2</span>")
           .arg(fontSize).arg(escaped.trimmed());
}

QString ChatBubbleWidget::formatMarkdown(const QString &content)
{
    QString result = content;
    
    // 处理代码块
    QRegularExpression codeBlockRegex("```([^\\n]*)\\n([\\s\\S]*?)```");
    QRegularExpressionMatchIterator it = codeBlockRegex.globalMatch(result);
    
    QVector<QPair<int, int>> positions;
    QStringList replacements;
    
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        QString lang = match.captured(1).trimmed();
        QString code = match.captured(2);
        
        // HTML转义
        code.replace("&", "&amp;");
        code.replace("<", "&lt;");
        code.replace(">", "&gt;");
        
        // 应用语法高亮（C++/C）
        if (lang.isEmpty() || lang == "cpp" || lang == "c++" || lang == "c") {
            code = applyCppSyntaxHighlight(code);
        }
        
        int fontSize = qRound(11 * m_fontScale);
        QString langLabel = lang.isEmpty() ? "代码" : lang;
        
        // 使用 table 布局代码块，确保正确换行
        QString codeHtml = QString(
            "<table cellpadding='0' cellspacing='0' style='width: 100%%; margin: 6px 0; "
            "background-color: #1e1e1e; border-collapse: collapse; "
            "border-left: 3px solid #007acc; table-layout: fixed;'>"
            "<tr><td style='padding: 4px 8px; background-color: #252525; border-bottom: 1px solid #3a3a3a;'>"
            "<span style='color: #9cdcfe; font-size: %1pt; font-weight: bold;'>%2</span></td></tr>"
            "<tr><td style='padding: 8px 12px; background-color: #1e1e1e;'>"
            "<pre style='margin: 0; padding: 0; background-color: #1e1e1e; "
            "font-family: Consolas, Courier New, monospace; "
            "font-size: %3pt; line-height: 1.4; "
            "white-space: pre-wrap; word-wrap: break-word; color: #d4d4d4;'>%4</pre>"
            "</td></tr></table>"
        ).arg(fontSize - 2).arg(langLabel).arg(fontSize).arg(code);
        
        positions.append(qMakePair(match.capturedStart(), match.capturedEnd()));
        replacements.append(codeHtml);
    }
    
    // 从后往前替换代码块
    for (int i = positions.size() - 1; i >= 0; --i) {
        result.replace(positions[i].first, positions[i].second - positions[i].first, 
                      QString("__CODE_%1__").arg(i));
    }
    
    // 处理数学公式（在HTML转义之前）
    QVector<QPair<int, int>> mathPositions;
    QStringList mathReplacements;
    
    // 块级数学公式 $$...$$
    QRegularExpression blockMathRegex("\\$\\$([^$]+)\\$\\$");
    QRegularExpressionMatchIterator mathIt = blockMathRegex.globalMatch(result);
    while (mathIt.hasNext()) {
        QRegularExpressionMatch match = mathIt.next();
        QString formula = match.captured(1).trimmed();
        
        // 简单渲染：使用斜体和特殊颜色
        QString mathHtml = QString(
            "<div style='margin: 8px 0; padding: 8px 12px; background: #2a2a2a; "
            "border-left: 3px solid #ffd700; border-radius: 4px;'>"
            "<span style='font-family: \"Times New Roman\", serif; font-style: italic; "
            "font-size: 11pt; color: #ffd700;'>%1</span>"
            "</div>"
        ).arg(formula);
        
        mathPositions.append(qMakePair(match.capturedStart(), match.capturedEnd()));
        mathReplacements.append(mathHtml);
    }
    
    // 从后往前替换块级数学公式
    for (int i = mathPositions.size() - 1; i >= 0; --i) {
        result.replace(mathPositions[i].first, 
                      mathPositions[i].second - mathPositions[i].first, 
                      QString("__MATH_BLOCK_%1__").arg(i));
    }
    
    // 行内数学公式 $...$（但不匹配 $$）
    QVector<QPair<int, int>> inlineMathPositions;
    QStringList inlineMathReplacements;
    
    QRegularExpression inlineMathRegex("(?<!\\$)\\$([^$\\n]+)\\$(?!\\$)");
    QRegularExpressionMatchIterator inlineMathIt = inlineMathRegex.globalMatch(result);
    while (inlineMathIt.hasNext()) {
        QRegularExpressionMatch match = inlineMathIt.next();
        QString formula = match.captured(1).trimmed();
        
        // 行内公式：使用斜体
        QString mathHtml = QString(
            "<span style='font-family: \"Times New Roman\", serif; font-style: italic; "
            "color: #ffd700; background: #2a2a2a; padding: 2px 6px; border-radius: 3px;'>%1</span>"
        ).arg(formula);
        
        inlineMathPositions.append(qMakePair(match.capturedStart(), match.capturedEnd()));
        inlineMathReplacements.append(mathHtml);
    }
    
    // 从后往前替换行内数学公式
    for (int i = inlineMathPositions.size() - 1; i >= 0; --i) {
        result.replace(inlineMathPositions[i].first, 
                      inlineMathPositions[i].second - inlineMathPositions[i].first, 
                      QString("__MATH_INLINE_%1__").arg(i));
    }
    
    // 转义 HTML
    result.replace("&", "&amp;");
    result.replace("<", "&lt;");
    result.replace(">", "&gt;");
    
    // Markdown 格式化 - 优化间距和可读性
    // 行内代码（支持单反引号）
    result.replace(QRegularExpression("`([^`]+)`"), 
                  "<code style='font-family: Consolas; font-weight: bold; color: #fff;'>\\1</code>");
    
    // 粗体（支持 **text** 和 __text__）
    result.replace(QRegularExpression("\\*\\*([^\\*]+)\\*\\*"), 
                  "<b style='color: #ffd700; letter-spacing: normal;'>\\1</b>");
    result.replace(QRegularExpression("__([^_]+)__"), 
                  "<b style='color: #ffd700; letter-spacing: normal;'>\\1</b>");
    
    // 斜体已禁用 - 不处理 *text* 和 _text_
    
    // 删除线（支持 ~~text~~）
    result.replace(QRegularExpression("~~([^~]+)~~"), 
                  "<s style='color: #888;'>\\1</s>");
    
    // 标题：添加适当的上下间距
    result.replace(QRegularExpression("\\n*^### (.+)$\\n*", QRegularExpression::MultilineOption), 
                  "<div style='color: #ffd700; font-size: 11.5pt; font-weight: bold; margin: 8px 0 4px 0; letter-spacing: normal;'>\\1</div>");
    result.replace(QRegularExpression("\\n*^## (.+)$\\n*", QRegularExpression::MultilineOption), 
                  "<div style='color: #ffd700; font-size: 12pt; font-weight: bold; margin: 10px 0 5px 0; letter-spacing: normal;'>\\1</div>");
    
    // 列表项：添加小间距
    result.replace(QRegularExpression("\\n*^(\\d+)\\. (.+)$\\n*", QRegularExpression::MultilineOption), 
                  "<div style='margin: 2px 0 2px 12px;'><span style='color: #ffd700; letter-spacing: normal;'>\\1.</span> \\2</div>");
    result.replace(QRegularExpression("\\n*^- (.+)$\\n*", QRegularExpression::MultilineOption), 
                  "<div style='margin: 2px 0 2px 12px;'><span style='color: #ffd700; letter-spacing: normal;'>•</span> \\1</div>");
    
    // 移除所有连续换行，只保留单个空格作为分隔
    result.replace(QRegularExpression("\\n+"), " ");
    
    // 清理多余空格
    result.replace(QRegularExpression(" +"), " ");
    
    // 移除首尾空格（重要！避免额外高度）
    result = result.trimmed();
    
    // 恢复数学公式
    for (int i = 0; i < mathReplacements.size(); ++i) {
        result.replace(QString("__MATH_BLOCK_%1__").arg(i), mathReplacements[i]);
    }
    for (int i = 0; i < inlineMathReplacements.size(); ++i) {
        result.replace(QString("__MATH_INLINE_%1__").arg(i), inlineMathReplacements[i]);
    }
    
    // 恢复代码块
    for (int i = 0; i < replacements.size(); ++i) {
        result.replace(QString("__CODE_%1__").arg(i), replacements[i]);
    }
    
    int fontSize = qRound(11 * m_fontScale);
    // AI 消息：行间距 1.5，与用户消息一致
    // 注意：不要在div末尾留空格或换行
    return QString("<div style='color: #f0f0f0; font-size: %1pt; line-height: 1.5; letter-spacing: normal;'>%2</div>")
           .arg(fontSize).arg(result.trimmed());
}

void ChatBubbleWidget::forceUpdate()
{
    setContent(m_content);
}

bool ChatBubbleWidget::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_textBrowser->viewport() && event->type() == QEvent::Wheel) {
        QWheelEvent *wheelEvent = static_cast<QWheelEvent*>(event);
        
        // Ctrl+滚轮：缩放字体
        if (wheelEvent->modifiers() & Qt::ControlModifier) {
            QWidget *parent = parentWidget();
            while (parent) {
                QScrollArea *scrollArea = qobject_cast<QScrollArea*>(parent->parentWidget());
                if (scrollArea) {
                    QApplication::sendEvent(scrollArea->viewport(), event);
                    return true;
                }
                parent = parent->parentWidget();
            }
            return true;
        }
        
        // 普通滚轮：检查是否需要气泡内部滚动
        QScrollBar *scrollBar = m_textBrowser->verticalScrollBar();
        if (scrollBar && scrollBar->isVisible()) {
            int delta = wheelEvent->angleDelta().y();
            int currentValue = scrollBar->value();
            
            // 向上滚动且未到顶部，或向下滚动且未到底部
            bool canScrollUp = (delta > 0 && currentValue > scrollBar->minimum());
            bool canScrollDown = (delta < 0 && currentValue < scrollBar->maximum());
            
            if (canScrollUp || canScrollDown) {
                // 气泡内部可以滚动，让QTextBrowser处理
                return false;
            }
        }
        
        // 气泡内部不能滚动，传递给父widget
        if (parentWidget()) {
            QApplication::sendEvent(parentWidget(), event);
        }
        return true;
    }
    
    return QWidget::eventFilter(obj, event);
}

void ChatBubbleWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    
    // 宽度变化时重新计算高度
    if (event->oldSize().width() != event->size().width() && event->size().width() > 0) {
        adjustHeight();
    }
}

void ChatBubbleWidget::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    QRectF bubbleRect = rect().adjusted(m_isUser ? 5 : 0, 5, m_isUser ? 0 : -5, -5);
    
    QPainterPath path;
    path.addRoundedRect(bubbleRect, 12, 12);
    
    // 阴影
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0, 0, 0, 30));
    painter.drawPath(path.translated(0, 2));
    
    // 背景
    painter.setBrush(m_isUser ? QColor(102, 0, 0) : QColor(68, 0, 0));
    painter.drawPath(path);
    
    // 边框
    QLinearGradient borderGradient(bubbleRect.topLeft(), bubbleRect.bottomLeft());
    if (m_isUser) {
        borderGradient.setColorAt(0, QColor(170, 0, 0));
        borderGradient.setColorAt(1, QColor(102, 0, 0));
    } else {
        borderGradient.setColorAt(0, QColor(136, 0, 0));
        borderGradient.setColorAt(1, QColor(68, 0, 0));
    }
    painter.setPen(QPen(QBrush(borderGradient), 2.0));
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(path);
    
    QWidget::paintEvent(event);
}

QString ChatBubbleWidget::applyCppSyntaxHighlight(const QString &code)
{
    QString result = code;
    
    // C++ 关键字
    QStringList keywords = {
        "int", "float", "double", "char", "bool", "void", "long", "short", "unsigned", "signed",
        "if", "else", "for", "while", "do", "switch", "case", "default", "break", "continue",
        "return", "const", "static", "extern", "auto", "register", "volatile",
        "struct", "union", "enum", "typedef", "sizeof",
        "class", "public", "private", "protected", "virtual", "friend", "inline",
        "new", "delete", "this", "operator", "namespace", "using", "template", "typename",
        "try", "catch", "throw", "true", "false", "nullptr"
    };
    
    // 预处理指令
    QStringList preprocessor = {
        "#include", "#define", "#ifdef", "#ifndef", "#endif", "#if", "#else", "#elif", "#pragma"
    };
    
    // 标准库函数和类型
    QStringList stdlib = {
        "std", "cout", "cin", "endl", "string", "vector", "map", "set", "list", "queue",
        "stack", "pair", "make_pair", "sort", "find", "push_back", "size", "empty",
        "printf", "scanf", "malloc", "free", "memset", "memcpy", "strlen", "strcpy"
    };
    
    // 1. 高亮字符串（绿色）
    result.replace(QRegularExpression("\"([^\"]*)\""), 
                   "<span style='color: #ce9178;'>\"\\1\"</span>");
    
    // 2. 高亮字符（绿色）
    result.replace(QRegularExpression("'([^']*)'"), 
                   "<span style='color: #ce9178;'>'\\1'</span>");
    
    // 3. 高亮注释（绿色）
    result.replace(QRegularExpression("//(.*)"), 
                   "<span style='color: #6a9955;'>//\\1</span>");
    
    // 4. 高亮数字（浅绿色）
    result.replace(QRegularExpression("\\b(\\d+\\.?\\d*)\\b"), 
                   "<span style='color: #b5cea8;'>\\1</span>");
    
    // 5. 高亮预处理指令（紫色）
    for (const QString &prep : preprocessor) {
        result.replace(QRegularExpression(QString("\\b(%1)\\b").arg(QRegularExpression::escape(prep))),
                      QString("<span style='color: #c586c0;'>%1</span>").arg(prep));
    }
    
    // 6. 高亮关键字（蓝色）
    for (const QString &keyword : keywords) {
        result.replace(QRegularExpression(QString("\\b(%1)\\b").arg(keyword)),
                      QString("<span style='color: #569cd6;'>%1</span>").arg(keyword));
    }
    
    // 7. 高亮标准库（青色）
    for (const QString &lib : stdlib) {
        result.replace(QRegularExpression(QString("\\b(%1)\\b").arg(lib)),
                      QString("<span style='color: #4ec9b0;'>%1</span>").arg(lib));
    }
    
    // 8. 高亮函数调用（黄色）
    result.replace(QRegularExpression("\\b([a-zA-Z_][a-zA-Z0-9_]*)\\s*\\("),
                   "<span style='color: #dcdcaa;'>\\1</span>(");
    
    return result;
}
