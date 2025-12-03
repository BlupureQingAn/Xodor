#include "ChatBubbleDelegate.h"
#include <QPainterPath>
#include <QApplication>
#include <QTextDocument>
#include <QAbstractTextDocumentLayout>

ChatBubbleDelegate::ChatBubbleDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
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
    
    // 绘制内容
    painter->setPen(Qt::white);
    QFont contentFont = painter->font();
    contentFont.setPointSize(11);
    painter->setFont(contentFont);
    
    QRectF contentRect = bubbleRect.adjusted(16, 38, -16, -12);
    painter->drawText(contentRect, Qt::AlignLeft | Qt::TextWordWrap, content);
    
    painter->restore();
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
    font.setPointSize(11);
    QFontMetrics fm(font);
    
    QRect boundingRect = fm.boundingRect(QRect(0, 0, maxWidth - 32, 10000),
                                         Qt::TextWordWrap | Qt::AlignLeft,
                                         text);
    
    return QSize(boundingRect.width(), boundingRect.height());
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
