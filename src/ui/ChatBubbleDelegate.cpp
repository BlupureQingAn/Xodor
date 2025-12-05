#include "ChatBubbleDelegate.h"
#include <QPainterPath>
#include <QApplication>
#include <QTextDocument>
#include <QAbstractTextDocumentLayout>
#include <QRegularExpression>
#include <QLinearGradient>
#include <QPen>
#include <QFontMetrics>
#include <QtMath>

ChatBubbleDelegate::ChatBubbleDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
    , m_fontScale(1.0)
{
}

void ChatBubbleDelegate::setFontScale(qreal scale)
{
    if (scale < 0.5) scale = 0.5;  // 最小50%
    if (scale > 2.0) scale = 2.0;  // 最大200%
    
    if (qAbs(m_fontScale - scale) > 0.01) {
        m_fontScale = scale;
        emit sizeChanged();
    }
}

void ChatBubbleDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                                const QModelIndex &index) const
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    
    QString role = index.data(Qt::UserRole).toString();
    QString content = index.data(Qt::DisplayRole).toString();
    QString timestamp = index.data(Qt::UserRole + 1).toString();
    
    bool isUser = (role == "user");
    
    // 计算气泡大小和位置
    int maxWidth = option.rect.width() * 0.65;
    QSize contentSize = calculateSize(content, role, maxWidth);
    
    int bubbleWidth = qMin(contentSize.width() + 40, maxWidth);
    int bubbleHeight = contentSize.height() + 60;
    
    QRectF bubbleRect;
    if (isUser) {
        // 用户消息靠右
        bubbleRect = QRectF(option.rect.right() - bubbleWidth - 10,
                           option.rect.top() + 10,
                           bubbleWidth,
                           bubbleHeight);
    } else {
        // AI消息靠左
        bubbleRect = QRectF(option.rect.left() + 10,
                           option.rect.top() + 10,
                           bubbleWidth,
                           bubbleHeight);
    }
    
    // 绘制气泡背景
    QColor bubbleColor = isUser ? QColor(37, 99, 235) : QColor(5, 150, 105);
    drawBubble(painter, bubbleRect, bubbleColor, isUser);
    
    // 绘制头部（名称和时间）
    painter->setPen(QColor(191, 219, 254));
    QFont headerFont = painter->font();
    headerFont.setPointSize(10);
    painter->setFont(headerFont);
    
    QString header = isUser ? QString("%1 你").arg(timestamp) : QString("🤖 AI导师 %1").arg(timestamp);
    QRectF headerRect = bubbleRect.adjusted(16, 12, -16, 0);
    headerRect.setHeight(20);
    
    if (isUser) {
        painter->drawText(headerRect, Qt::AlignRight | Qt::AlignVCenter, header);
    } else {
        painter->drawText(headerRect, Qt::AlignLeft | Qt::AlignVCenter, header);
    }
    
    // 绘制内容（使用QTextDocument支持HTML格式）
    QFont contentFont = painter->font();
    contentFont.setPointSize(qRound(11 * m_fontScale));
    
    QRectF contentRect = bubbleRect.adjusted(16, 38, -16, -12);
    
    QTextDocument doc;
    doc.setDefaultFont(contentFont);
    doc.setTextWidth(contentRect.width());
    doc.setDocumentMargin(0);  // 减少文档边距
    
    // 如果是AI消息，格式化Markdown
    if (!isUser && role == "assistant") {
        QString formattedContent = formatMarkdown(content);
        doc.setHtml(formattedContent);
    } else {
        doc.setPlainText(content);
    }
    
    painter->translate(contentRect.topLeft());
    QRectF clip(0, 0, contentRect.width(), contentRect.height());
    doc.drawContents(painter, clip);
    
    painter->restore();
}

QString ChatBubbleDelegate::formatMarkdown(const QString &content) const
{
    QString result = content;
    
    // 先转义HTML特殊字符（但保留换行符）
    result.replace("&", "&amp;");
    result.replace("<", "&lt;");
    result.replace(">", "&gt;");
    
    // 处理代码块 ```language\ncode\n``` （必须在其他处理之前）
    QRegularExpression codeBlockRegex("```([^\\n]*)\\n([\\s\\S]*?)```");
    QRegularExpressionMatchIterator it = codeBlockRegex.globalMatch(result);
    
    QVector<QPair<int, int>> codeBlockPositions;
    QStringList codeBlockReplacements;
    
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        QString language = match.captured(1).trimmed();
        QString code = match.captured(2);
        
        // 简单的C++语法高亮
        QString highlightedCode = code;
        if (language.isEmpty() || language == "cpp" || language == "c++") {
            // 关键字高亮（紫色）
            QStringList keywords = {"int", "char", "float", "double", "void", "bool", "string",
                                   "if", "else", "for", "while", "do", "switch", "case", "break", "continue",
                                   "return", "const", "static", "class", "struct", "namespace", "using",
                                   "include", "define", "typedef", "template", "typename", "public", "private", "protected"};
            for (const QString &kw : keywords) {
                highlightedCode.replace(QRegularExpression(QString("\\b%1\\b").arg(kw)),
                                       QString("<span style='color: #c586c0; font-weight: bold;'>%1</span>").arg(kw));
            }
            
            // 字符串高亮（橙色）
            highlightedCode.replace(QRegularExpression("\"([^\"]*)\""),
                                   "<span style='color: #ce9178;'>\"\\1\"</span>");
            
            // 数字高亮（浅绿色）
            highlightedCode.replace(QRegularExpression("\\b(\\d+)\\b"),
                                   "<span style='color: #b5cea8;'>\\1</span>");
            
            // 注释高亮（绿色）
            highlightedCode.replace(QRegularExpression("//(.*)$", QRegularExpression::MultilineOption),
                                   "<span style='color: #6a9955;'>//\\1</span>");
            
            // 预处理器高亮（青色）
            highlightedCode.replace(QRegularExpression("^(#.*)$", QRegularExpression::MultilineOption),
                                   "<span style='color: #4ec9b0;'>\\1</span>");
        }
        
        // 代码块样式：圆角、纯色灰背景
        int fontSize = qRound(11 * m_fontScale);
        QString codeHtml = QString(
            "<div style='background: #2d2d2d; padding: 12px; border-radius: 8px; "
            "margin: 8px 0; border: 1px solid #3d3d3d;'>"
            "<div style='color: #858585; font-size: %1pt; margin-bottom: 6px; font-weight: bold;'>%2</div>"
            "<pre style='margin: 0; padding: 0; "
            "font-family: \"Consolas\", \"Courier New\", monospace; "
            "font-size: %3pt; line-height: 1.3; "
            "white-space: pre-wrap; word-wrap: break-word; color: #d4d4d4;'>%4</pre>"
            "</div>"
        ).arg(fontSize - 2).arg(language.isEmpty() ? "代码" : language).arg(fontSize).arg(highlightedCode);
        
        codeBlockPositions.append(qMakePair(match.capturedStart(), match.capturedEnd()));
        codeBlockReplacements.append(codeHtml);
    }
    
    // 从后往前替换，避免位置偏移
    for (int i = codeBlockPositions.size() - 1; i >= 0; --i) {
        result.replace(codeBlockPositions[i].first, 
                      codeBlockPositions[i].second - codeBlockPositions[i].first, 
                      codeBlockReplacements[i]);
    }
    
    // 处理行内代码 `code`
    result.replace(QRegularExpression("`([^`]+)`"), 
                  "<code style='background: rgba(0,0,0,0.5); padding: 2px 5px; border-radius: 3px; "
                  "font-family: \"Consolas\", \"Courier New\", monospace; font-size: 10.5pt; color: #ffd700;'>\\1</code>");
    
    // 处理加粗 **text**
    result.replace(QRegularExpression("\\*\\*([^\\*]+)\\*\\*"), 
                  "<b style='color: #ffd700; font-weight: bold;'>\\1</b>");
    
    // 处理斜体 *text*
    result.replace(QRegularExpression("\\*([^\\*]+)\\*"), 
                  "<i style='color: #e8e8e8;'>\\1</i>");
    
    // 处理标题 ### text
    result.replace(QRegularExpression("^### (.+)$", QRegularExpression::MultilineOption), 
                  "<div style='color: #ffd700; margin: 6px 0 3px 0; font-size: 11.5pt; font-weight: bold;'>\\1</div>");
    result.replace(QRegularExpression("^## (.+)$", QRegularExpression::MultilineOption), 
                  "<div style='color: #ffd700; margin: 8px 0 4px 0; font-size: 12pt; font-weight: bold;'>\\1</div>");
    
    // 处理有序列表 1. text
    result.replace(QRegularExpression("^(\\d+)\\. (.+)$", QRegularExpression::MultilineOption), 
                  "<div style='margin: 1px 0; padding-left: 12px;'><span style='color: #ffd700; font-weight: bold;'>\\1.</span> \\2</div>");
    
    // 处理无序列表 - text
    result.replace(QRegularExpression("^- (.+)$", QRegularExpression::MultilineOption), 
                  "<div style='margin: 1px 0; padding-left: 12px;'><span style='color: #ffd700; font-weight: bold;'>•</span> \\1</div>");
    
    // 处理换行（在最后处理）
    result.replace("\n", "<br>");
    
    // 设置基础样式，统一字体和行高（行高降到1.1，更紧凑）
    int fontSize = qRound(11 * m_fontScale);
    int codeSize = qRound(11 * m_fontScale);
    return QString("<div style='color: #f0f0f0; line-height: 1.1; "
                  "font-family: \"Microsoft YaHei\", \"Segoe UI\", Arial, sans-serif; "
                  "font-size: %1pt; margin: 0; padding: 0;'>%2</div>").arg(fontSize).arg(result);
}

QSize ChatBubbleDelegate::sizeHint(const QStyleOptionViewItem &option,
                                    const QModelIndex &index) const
{
    QString content = index.data(Qt::DisplayRole).toString();
    QString role = index.data(Qt::UserRole).toString();
    
    int maxWidth = option.rect.width() * 0.65;
    QSize contentSize = calculateSize(content, role, maxWidth);
    
    return QSize(option.rect.width(), contentSize.height() + 80);
}

QSize ChatBubbleDelegate::calculateSize(const QString &text, const QString &role, int maxWidth) const
{
    QFont font;
    font.setPointSize(qRound(11 * m_fontScale));
    
    // 如果是AI消息，使用QTextDocument计算HTML内容的大小
    if (role == "assistant") {
        QTextDocument doc;
        doc.setDefaultFont(font);
        doc.setTextWidth(maxWidth - 32);
        doc.setDocumentMargin(0);
        
        QString formattedContent = formatMarkdown(text);
        doc.setHtml(formattedContent);
        
        QSize size = doc.size().toSize();
        return QSize(maxWidth - 32, size.height());
    } else {
        // 用户消息使用简单的文本计算
        QFontMetrics fm(font);
        QRect boundingRect = fm.boundingRect(QRect(0, 0, maxWidth - 32, 10000),
                                             Qt::TextWordWrap | Qt::AlignLeft,
                                             text);
        return QSize(boundingRect.width(), boundingRect.height());
    }
}

void ChatBubbleDelegate::drawBubble(QPainter *painter, const QRectF &rect,
                                     const QColor &color, bool isUser) const
{
    QPainterPath path;
    qreal radius = 20.0;
    
    // 创建圆角矩形路径
    path.addRoundedRect(rect, radius, radius);
    
    // 绘制阴影
    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(0, 0, 0, 30));
    painter->drawPath(path.translated(0, 2));
    
    // 绘制气泡
    QLinearGradient gradient(rect.topLeft(), rect.bottomLeft());
    if (isUser) {
        gradient.setColorAt(0, QColor(59, 130, 246));
        gradient.setColorAt(1, QColor(37, 99, 235));
    } else {
        gradient.setColorAt(0, QColor(16, 185, 129));
        gradient.setColorAt(1, QColor(5, 150, 105));
    }
    
    painter->setBrush(gradient);
    painter->drawPath(path);
    
    // 绘制边框
    painter->setPen(QPen(color.darker(120), 2));
    painter->setBrush(Qt::NoBrush);
    painter->drawPath(path);
}
