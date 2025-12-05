#include "ChatBubbleWidget.h"
#include <QPainter>
#include <QPainterPath>
#include <QRegularExpression>
#include <QScrollArea>
#include <QWheelEvent>
#include <QApplication>
#include <QAbstractTextDocumentLayout>

ChatBubbleWidget::ChatBubbleWidget(const QString &content, bool isUser, QWidget *parent)
    : QWidget(parent)
    , m_content(content)
    , m_isUser(isUser)
    , m_fontScale(1.0)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    
    QVBoxLayout *layout = new QVBoxLayout(this);
    // 减小布局边距：左右保持，上下改为6px
    layout->setContentsMargins(isUser ? 10 : 5, 6, isUser ? 5 : 10, 6);
    
    m_textBrowser = new QTextBrowser(this);
    m_textBrowser->setOpenExternalLinks(false);
    m_textBrowser->setReadOnly(true);
    m_textBrowser->setFrameShape(QFrame::NoFrame);
    m_textBrowser->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_textBrowser->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    // 关键：让 QTextBrowser 根据 widget 宽度自动换行
    m_textBrowser->setLineWrapMode(QTextEdit::WidgetWidth);
    m_textBrowser->setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    m_textBrowser->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    m_textBrowser->viewport()->installEventFilter(this);
    
    // 减小文档边距：从8px改为4px
    m_textBrowser->document()->setDocumentMargin(4);
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
    )");
    
    layout->addWidget(m_textBrowser);
    
    // 设置内容
    setContent(content);
}

void ChatBubbleWidget::setContent(const QString &content)
{
    m_content = content;
    
    QString html;
    if (m_isUser) {
        html = formatUserMessage(content);
    } else {
        html = formatMarkdown(content);
    }
    
    m_textBrowser->setHtml(html);
    
    // 调试：检查HTML末尾是否有多余空白
    if (html.length() > content.length() * 2) {
        qDebug() << "[ChatBubbleWidget] Warning: HTML length" << html.length() 
                 << "is much larger than content length" << content.length();
    }
    
    // 让 Qt 自动计算高度
    adjustHeight();
}

void ChatBubbleWidget::adjustHeight()
{
    QTextDocument *doc = m_textBrowser->document();
    doc->setTextWidth(m_textBrowser->viewport()->width());
    
    // 使用 documentLayout 获取更精确的高度
    QAbstractTextDocumentLayout *layout = doc->documentLayout();
    int docHeight = qRound(layout->documentSize().height());
    int margin = doc->documentMargin();
    
    // QTextBrowser 的高度 = 文档高度 + 文档边距
    int textBrowserHeight = docHeight + margin * 2;
    m_textBrowser->setFixedHeight(textBrowserHeight);
    
    // Widget 的高度 = QTextBrowser 高度 + 布局边距（上下各6px）
    int widgetHeight = textBrowserHeight + 12;
    setMinimumHeight(widgetHeight);
    setMaximumHeight(widgetHeight);  // 设置最大高度，避免多余空间
    
    // 调试日志（可选）
    // qDebug() << "[ChatBubbleWidget] Adjusted height - doc:" << docHeight 
    //          << "margin:" << margin << "final:" << widgetHeight;
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
    int fontSize = qRound(11 * m_fontScale);
    
    // 去除首尾的空白字符和换行符
    QString trimmed = content.trimmed();
    
    QString escaped = trimmed;
    escaped.replace("&", "&amp;");
    escaped.replace("<", "&lt;");
    escaped.replace(">", "&gt;");
    escaped.replace("\n", "<br>");
    
    // 再次trim，确保没有首尾空白
    escaped = escaped.trimmed();
    
    // 用户消息：行间距 1.5，更舒适
    return QString("<div style='color: #f0f0f0; font-size: %1pt; line-height: 1.5; letter-spacing: normal;'>%2</div>")
           .arg(fontSize).arg(escaped);
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
        code.replace("&", "&amp;");
        code.replace("<", "&lt;");
        code.replace(">", "&gt;");
        
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
    
    // 斜体（支持 *text* 和 _text_）
    result.replace(QRegularExpression("\\*([^\\*]+)\\*"), 
                  "<i style='color: #e8e8e8;'>\\1</i>");
    result.replace(QRegularExpression("(?<!_)_([^_]+)_(?!_)"), 
                  "<i style='color: #e8e8e8;'>\\1</i>");
    
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
        
        // 传递给父 widget 处理滚动
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
