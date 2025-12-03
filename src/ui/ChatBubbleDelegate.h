#ifndef CHATBUBBLEDELEGATE_H
#define CHATBUBBLEDELEGATE_H

#include <QStyledItemDelegate>
#include <QPainter>
#include <QTextDocument>

class ChatBubbleDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit ChatBubbleDelegate(QObject *parent = nullptr);
    
    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
    
    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override;
    
private:
    QSize calculateSize(const QString &text, const QString &role, int maxWidth) const;
    void drawBubble(QPainter *painter, const QRectF &rect, const QColor &color, bool isUser) const;
};

#endif // CHATBUBBLEDELEGATE_H
